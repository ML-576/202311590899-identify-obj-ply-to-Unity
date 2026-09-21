/*
  ModelBridge.cpp —— Unity 原生插件桥接实现 (新建文件, 2026)
  ------------------------------------------------------------------
  OBJ 路线: 使用本目录提供的 OBJ 类(glmReadOBJ/glmUnitize/glmFacetNormals/
            glmVertexNormals 完整流程)解析, 再把 GLMmodel 数据展开为
            Unity 可直接使用的每顶点数组; 材质取自 glm 解析的 MTL,
            纹理文件名(map_Kd)由本桥接层补充解析(原glm不保存该项)。
  PLY 路线: 使用 hply(get_nverts_nfaces 预读头部数量) + ReadPlyInCore
            (read_ply_incore 读取数据, 支持ASCII/二进制大端/小端,
            多边形面自动三角化), 法线优先取文件中的 nx/ny/nz,
            缺失时用 RawMesh::normal_for_v() 计算; 顶点颜色归一化输出。
  所有解析过程不调用 exit(), 错误以返回码/错误字符串上报给 Unity。
*/
#include "pch.h"
#include "ModelBridge.h"

#include <string>
#include <vector>
#include <cstring>
#include <cstdio>
#include <ctime>
#include <new>
#include <algorithm>

#include "OBJ.h"
#include "ReadPly.h"

/* ------------------------------------------------------------------ */
/* 内部数据结构                                                        */
/* ------------------------------------------------------------------ */

struct MR_SubMeshData { int materialIndex; int indexStart; int indexCount; };

struct MR_MaterialData
{
    std::string name;
    float diffuse[4];
    float ambient[4];
    float specular[4];
    float shininess;
    bool  hasTexture;
    std::string texturePath;
    MR_MaterialData()
    {
        diffuse[0]=diffuse[1]=diffuse[2]=0.8f; diffuse[3]=1.0f;
        ambient[0]=ambient[1]=ambient[2]=0.2f; ambient[3]=1.0f;
        specular[0]=specular[1]=specular[2]=0.0f; specular[3]=1.0f;
        shininess=32.0f; hasTexture=false;
    }
};

struct ModelData
{
    std::vector<float> positions;   /* 3/vertex */
    std::vector<float> normals;     /* 3/vertex */
    std::vector<float> uvs;         /* 2/vertex */
    std::vector<float> colors;      /* 4/vertex, 可为空 */
    std::vector<int>   indices;     /* 3/triangle */
    std::vector<MR_SubMeshData> subMeshes;
    std::vector<MR_MaterialData> materials;
    MR_ModelInfo info;
    ModelData() { memset(&info, 0, sizeof(info)); }
};

static std::vector<ModelData*> g_models;
static void* g_mutex = NULL;      /* 惰性初始化的临界区/互斥锁 */
static char  g_lastError[1024] = {0};

#ifdef _WIN32
#include <windows.h>
static void MR_Lock()   { if(!g_mutex){ g_mutex = (void*)new CRITICAL_SECTION; InitializeCriticalSection((CRITICAL_SECTION*)g_mutex);} EnterCriticalSection((CRITICAL_SECTION*)g_mutex); }
static void MR_Unlock() { LeaveCriticalSection((CRITICAL_SECTION*)g_mutex); }
#else
#include <pthread.h>
static void MR_Lock()   { if(!g_mutex){ g_mutex = (void*)new pthread_mutex_t; pthread_mutex_init((pthread_mutex_t*)g_mutex, NULL);} pthread_mutex_lock((pthread_mutex_t*)g_mutex); }
static void MR_Unlock() { pthread_mutex_unlock((pthread_mutex_t*)g_mutex); }
#endif

static void MR_SetError(const char* fmt, const char* arg1 = NULL)
{
    if (arg1)
        sprintf(g_lastError, fmt, arg1);
    else
        sprintf(g_lastError, "%s", fmt);
}

/* Windows下UTF-8路径打开文件(支持中文路径); 其他平台直接fopen */
static FILE* BridgeFOpen(const char* path, const char* mode)
{
#ifdef _WIN32
    wchar_t wpath[1024];
    wchar_t wmode[16];
    if (MultiByteToWideChar(CP_UTF8, 0, path, -1, wpath, 1024) > 0 &&
        MultiByteToWideChar(CP_UTF8, 0, mode, -1, wmode, 16) > 0)
        return _wfopen(wpath, wmode);
    return NULL;
#else
    return fopen(path, mode);
#endif
}

/* 取文件大小 */
static double BridgeFileSize(const char* path)
{
    FILE* f = BridgeFOpen(path, "rb");
    if (!f) return 0;
    fseek(f, 0, SEEK_END);
    long sz = ftell(f);
    fclose(f);
    return (double)sz;
}

/* 提取文件名(含扩展名) */
static void BridgeFileName(const char* path, char* out, int cap)
{
    const char* s1 = strrchr(path, '\\');
    const char* s2 = strrchr(path, '/');
    const char* s = (s1 && s2) ? (s1 > s2 ? s1 : s2) : (s1 ? s1 : s2);
    s = s ? s + 1 : path;
    strncpy(out, s, cap - 1);
    out[cap - 1] = 0;
}

/* 扩展名小写比较: 返回0表示相等 */
static bool BridgeExtIs(const char* path, const char* ext)
{
    size_t lp = strlen(path), le = strlen(ext);
    if (lp < le) return false;
    for (size_t i = 0; i < le; i++)
    {
        char a = path[lp - le + i];
        char b = ext[i];
        if (a >= 'A' && a <= 'Z') a += 32;
        if (a != b) return false;
    }
    return true;
}

/* ------------------------------------------------------------------ */
/* OBJ: 解析 MTL 中的 map_Kd 纹理文件名 (原 glm 不保存该信息)           */
/* ------------------------------------------------------------------ */
static void BridgeParseMTLTextures(const char* objPath, const char* mtllibName,
                                   std::vector<MR_MaterialData>& mats)
{
    if (!mtllibName || !mtllibName[0]) return;

    /* 拼接 mtl 绝对路径 = obj 所在目录 + mtllib 文件名 */
    std::string dir(objPath);
    size_t p1 = dir.find_last_of('\\');
    size_t p2 = dir.find_last_of('/');
    size_t p = std::string::npos;
    if (p1 != std::string::npos && p2 != std::string::npos) p = (p1 > p2 ? p1 : p2);
    else if (p1 != std::string::npos) p = p1;
    else if (p2 != std::string::npos) p = p2;
    std::string mtlPath = (p == std::string::npos) ? std::string(mtllibName)
                          : dir.substr(0, p + 1) + mtllibName;

    FILE* f = BridgeFOpen(mtlPath.c_str(), "r");
    if (!f)
    {
        sprintf(g_lastError + strlen(g_lastError),
                " [警告] 无法打开材质库文件 \"%s\", 已使用默认材质", mtllibName);
        return;
    }

    /* name -> 材质索引 映射 */
    char line[1024];
    char curName[128] = {0};
    while (fgets(line, sizeof(line), f))
    {
        char key[128] = {0}, val[512] = {0};
        if (sscanf(line, "%127s %511[^\r\n]", key, val) < 1) continue;

        if (!strcmp(key, "newmtl"))
        {
            strncpy(curName, val, sizeof(curName) - 1);
        }
        else if (!strcmp(key, "map_Kd") && curName[0])
        {
            /* 找到对应材质并填入纹理路径(转为绝对路径) */
            for (size_t i = 0; i < mats.size(); i++)
            {
                if (mats[i].name == curName)
                {
                    if (val[0] == '\0') break;
                    std::string tex = val;
                    /* 去掉首尾空格 */
                    size_t b = tex.find_first_not_of(" \t");
                    size_t e = tex.find_last_not_of(" \t");
                    if (b == std::string::npos) break;
                    tex = tex.substr(b, e - b + 1);
                    /* 已是绝对路径则直接用 */
                    if (tex.size() > 1 && (tex[1] == ':' || tex[0] == '/'))
                        mats[i].texturePath = tex;
                    else
                    {
                        std::string d2(objPath);
                        size_t q1 = d2.find_last_of('\\'), q2 = d2.find_last_of('/');
                        size_t q = std::string::npos;
                        if (q1 != std::string::npos && q2 != std::string::npos) q = (q1 > q2 ? q1 : q2);
                        else if (q1 != std::string::npos) q = q1;
                        else if (q2 != std::string::npos) q = q2;
                        mats[i].texturePath = (q == std::string::npos) ? tex
                                              : d2.substr(0, q + 1) + tex;
                    }
                    mats[i].hasTexture = true;
                    break;
                }
            }
        }
    }
    fclose(f);
}

/* ------------------------------------------------------------------ */
/* OBJ 加载                                                            */
/* ------------------------------------------------------------------ */
static int BridgeLoadOBJ(const char* path, ModelData* md)
{
    /* Windows 下统一为反斜杠, 保证 glmDirName 能取出目录 */
    std::string workPath(path);
#ifdef _WIN32
    std::replace(workPath.begin(), workPath.end(), '/', '\\');
#endif

    OBJ obj;
    obj.readOBJ(const_cast<char*>(workPath.c_str()));
    GLMmodel* m = obj.getGLMModel();
    if (!m || m->numvertices == 0 || m->numtriangles == 0)
    {
        MR_SetError("OBJ 解析失败: 文件内容不是有效的 Wavefront OBJ 格式", path);
        return MR_ERR_PARSE_FAILED;
    }

    md->info.vertexCount   = (int)m->numvertices;
    md->info.triangleCount = (int)m->numtriangles;
    strcpy(md->info.format, "OBJ");

    /* ---- 材质 ---- */
    md->materials.resize(m->nummaterials > 0 ? m->nummaterials : 1);
    for (unsigned int i = 0; i < md->materials.size() && i < m->nummaterials; i++)
    {
        MR_MaterialData& d = md->materials[i];
        GLMmaterial& s = m->materials[i];
        d.name = s.name ? s.name : ("material_" + std::to_string(i));
        for (int c = 0; c < 4; c++)
        {
            d.diffuse[c]  = s.diffuse[c];
            d.ambient[c]  = s.ambient[c];
            d.specular[c] = s.specular[c];
        }
        d.shininess = s.shininess;
    }
    BridgeParseMTLTextures(workPath.c_str(), m->mtllibname, md->materials);

    /* ---- 按组展开几何数据(OBJ索引为1-based) ---- */
    int corner = 0;
    for (GLMgroup* g = m->groups; g != NULL; g = g->next)
    {
        if (g->numtriangles == 0) continue;

        MR_SubMeshData sm;
        sm.materialIndex = (g->material >= 0 && g->material < (int)md->materials.size())
                           ? g->material : 0;
        sm.indexStart = (int)md->indices.size();
        sm.indexCount = g->numtriangles * 3;
        md->subMeshes.push_back(sm);

        for (unsigned int t = 0; t < g->numtriangles; t++)
        {
            GLMtriangle& tr = m->triangles[g->triangles[t]];
            for (int c = 0; c < 3; c++)
            {
                /* 顶点(1-based), 越界保护 */
                unsigned int v = tr.vindices[c];
                if (v == 0 || v > m->numvertices) v = 1;
                md->positions.push_back(m->vertices[3 * v + 0]);
                md->positions.push_back(m->vertices[3 * v + 1]);
                md->positions.push_back(m->vertices[3 * v + 2]);

                /* 法线: 优先顶点法线, 其次面法线, 最后默认 */
                float nx = 0.0f, ny = 1.0f, nz = 0.0f;
                if (m->numnormals > 0 && m->normals &&
                    tr.nindices[c] >= 1 && tr.nindices[c] <= m->numnormals)
                {
                    nx = m->normals[3 * tr.nindices[c] + 0];
                    ny = m->normals[3 * tr.nindices[c] + 1];
                    nz = m->normals[3 * tr.nindices[c] + 2];
                }
                else if (m->numfacetnorms > 0 && m->facetnorms &&
                         tr.findex >= 1 && tr.findex <= m->numfacetnorms)
                {
                    nx = m->facetnorms[3 * tr.findex + 0];
                    ny = m->facetnorms[3 * tr.findex + 1];
                    nz = m->facetnorms[3 * tr.findex + 2];
                }
                md->normals.push_back(nx);
                md->normals.push_back(ny);
                md->normals.push_back(nz);

                /* 纹理坐标(1-based); OBJ 的 v 轴原点在左下, Unity 在左上, 翻转 y */
                if (m->numtexcoords > 0 && m->texcoords &&
                    tr.tindices[c] >= 1 && tr.tindices[c] <= m->numtexcoords)
                {
                    md->uvs.push_back(m->texcoords[2 * tr.tindices[c] + 0]);
                    md->uvs.push_back(1.0f - m->texcoords[2 * tr.tindices[c] + 1]);
                }
                else
                {
                    md->uvs.push_back(0.0f);
                    md->uvs.push_back(0.0f);
                }

                md->indices.push_back(corner);
                corner++;
            }
        }
    }

    return MR_OK;
}

/* ------------------------------------------------------------------ */
/* PLY 加载                                                            */
/* ------------------------------------------------------------------ */
static void BridgePlyFormat(const char* path, char* out, int cap)
{
    strcpy(out, "PLY");
    FILE* f = BridgeFOpen(path, "rb");
    if (!f) return;
    char head[512] = {0};
    size_t n = fread(head, 1, sizeof(head) - 1, f);
    fclose(f);
    head[n] = 0;
    if (strstr(head, "format ascii"))            strcpy(out, "PLY ASCII");
    else if (strstr(head, "binary_big_endian"))  strcpy(out, "PLY BIN BE");
    else if (strstr(head, "binary_little_endian")) strcpy(out, "PLY BIN LE");
}

static int BridgeLoadPLY(const char* path, ModelData* md)
{
    /* 1. 校验文件头 */
    FILE* f = BridgeFOpen(path, "rb");
    if (!f)
    {
        MR_SetError("PLY 解析失败: 无法打开文件", path);
        return MR_ERR_FILE_NOT_FOUND;
    }
    char magic[4] = {0};
    fread(magic, 1, 3, f);
    fclose(f);
    if (strncmp(magic, "ply", 3) != 0)
    {
        MR_SetError("PLY 解析失败: 文件缺少 \"ply\" 头标识, 不是有效的 PLY 文件", path);
        return MR_ERR_PARSE_FAILED;
    }

    BridgePlyFormat(path, md->info.format, sizeof(md->info.format));

    /* 2. 预读头部获得顶点数/面数 (hply 提供) */
    HPLY plyHelper;
    int nvf[2] = {-1, -1};
    HPLY::clear_ply_error();
    plyHelper.get_nverts_nfaces(path, nvf);
    if (HPLY::get_ply_error() == 3)
    {
        MR_SetError("PLY 解析失败: 无法打开文件", path);
        return MR_ERR_FILE_NOT_FOUND;
    }
    if (HPLY::get_ply_error() == 4 || nvf[0] < 0)
    {
        MR_SetError("PLY 解析失败: 文件头无效(缺少 ply/element vertex 声明)", path);
        return MR_ERR_PARSE_FAILED;
    }
    if (nvf[1] < 0)
    {
        MR_SetError("PLY 解析失败: 文件头中缺少 element face 声明", path);
        return MR_ERR_PARSE_FAILED;
    }

    /* 3. RawMesh 按头数量分配, ReadPlyInCore 读取数据 */
    RawMesh rm;
    rm.init(nvf[0], nvf[1] > 0 ? nvf[1] : 0);

    ReadPlyInCore reader;
    f = BridgeFOpen(path, "rb");
    if (!f)
    {
        MR_SetError("PLY 解析失败: 无法打开文件", path);
        return MR_ERR_FILE_NOT_FOUND;
    }
    HPLY::clear_ply_error();
    reader.read_ply_incore(f, &rm);
    fclose(f);

    if (reader.errorCode != 0)
    {
        const char* what = "未知错误";
        switch (reader.errorCode)
        {
            case 1: what = "文件数据不完整(读取失败)"; break;
            case 2: what = "文件数据意外结束(截断)"; break;
            case 4: what = "文件头无效"; break;
            case 10: what = "顶点缺少 x/y/z 坐标"; break;
            case 11: what = "面元素缺少顶点索引属性"; break;
        }
        MR_SetError("PLY 解析失败: %s", what);
        return MR_ERR_PARSE_FAILED;
    }
    if (rm.nverts <= 0 || rm.nfaces <= 0)
    {
        MR_SetError("PLY 解析失败: 文件中没有可用的顶点或面数据", path);
        return MR_ERR_EMPTY_MESH;
    }

    md->info.vertexCount   = rm.nverts;
    md->info.triangleCount = rm.nfaces;

    /* 4. 几何数据(PLY顶点被面共享, 无需展开) */
    md->positions.resize((size_t)rm.nverts * 3);
    memcpy(md->positions.data(), rm.vlist, sizeof(float) * 3 * rm.nverts);

    /* 法线: 文件未提供时用面平均计算 */
    if (!reader.hasNormals)
        rm.normal_for_v();
    md->normals.resize((size_t)rm.nverts * 3);
    memcpy(md->normals.data(), rm.nlist, sizeof(float) * 3 * rm.nverts);

    /* 索引 */
    md->indices.resize((size_t)rm.nfaces * 3);
    for (int i = 0; i < rm.nfaces; i++)
    {
        md->indices[3 * i + 0] = rm.flist[i].v[0];
        md->indices[3 * i + 1] = rm.flist[i].v[1];
        md->indices[3 * i + 2] = rm.flist[i].v[2];
    }

    /* UV: PLY 常无纹理坐标, 填0 */
    md->uvs.assign((size_t)rm.nverts * 2, 0.0f);

    /* 顶点颜色(RGBA) */
    if (reader.hasColors && (int)reader.vertexColors.size() >= rm.nverts * 3)
    {
        md->colors.resize((size_t)rm.nverts * 4);
        for (int i = 0; i < rm.nverts; i++)
        {
            md->colors[4 * i + 0] = reader.vertexColors[3 * i + 0];
            md->colors[4 * i + 1] = reader.vertexColors[3 * i + 1];
            md->colors[4 * i + 2] = reader.vertexColors[3 * i + 2];
            md->colors[4 * i + 3] = 1.0f;
        }
        md->info.hasVertexColors = 1;
    }

    /* 单一子网格与默认材质 */
    MR_SubMeshData sm; sm.materialIndex = 0; sm.indexStart = 0; sm.indexCount = rm.nfaces * 3;
    md->subMeshes.push_back(sm);
    MR_MaterialData mat;
    mat.name = "PLY_Default";
    if (md->info.hasVertexColors)
    {
        mat.diffuse[0] = mat.diffuse[1] = mat.diffuse[2] = 1.0f; /* 用顶点色显示 */
    }
    md->materials.push_back(mat);

    return MR_OK;
}

/* ------------------------------------------------------------------ */
/* C 接口实现                                                          */
/* ------------------------------------------------------------------ */

MR_API int MR_LoadModel(const char* utf8Path)
{
    g_lastError[0] = 0;

    if (!utf8Path || !utf8Path[0])
    {
        MR_SetError("加载失败: 文件路径为空");
        return MR_ERR_NULL_PATH;
    }

    MR_Lock();

    ModelData* md = new (std::nothrow) ModelData();
    if (!md) { MR_Unlock(); MR_SetError("加载失败: 内存不足"); return MR_ERR_NO_MEMORY; }

    clock_t t0 = clock();
    int rc;
    if (BridgeExtIs(utf8Path, ".obj"))
        rc = BridgeLoadOBJ(utf8Path, md);
    else if (BridgeExtIs(utf8Path, ".ply"))
        rc = BridgeLoadPLY(utf8Path, md);
    else
    {
        MR_SetError("加载失败: 不支持的文件类型 \"%s\" (仅支持 .obj 和 .ply)", utf8Path);
        rc = MR_ERR_UNKNOWN_FORMAT;
    }

    if (rc != MR_OK)
    {
        delete md;
        MR_Unlock();
        return rc;
    }

    /* 完成概要信息 */
    md->info.meshVertexCount = (int)(md->positions.size() / 3);
    md->info.subMeshCount    = (int)md->subMeshes.size();
    md->info.materialCount   = (int)md->materials.size();
    BridgeFileName(utf8Path, md->info.name, sizeof(md->info.name));
    md->info.fileSizeBytes    = BridgeFileSize(utf8Path);
    md->info.loadTimeSeconds  = (double)(clock() - t0) / CLOCKS_PER_SEC;

    g_models.push_back(md);
    int handle = (int)g_models.size();   /* 1-based 句柄 */
    MR_Unlock();
    return handle;
}

static ModelData* MR_Get(int handle)
{
    if (handle <= 0 || handle > (int)g_models.size()) return NULL;
    return g_models[handle - 1];
}

MR_API int MR_GetModelInfo(int handle, MR_ModelInfo* outInfo)
{
    MR_Lock();
    ModelData* md = MR_Get(handle);
    if (!md || !outInfo) { MR_Unlock(); return MR_ERR_BAD_HANDLE; }
    *outInfo = md->info;
    MR_Unlock();
    return MR_OK;
}

/* 通用拷贝模板 */
template <typename T>
static int MR_CopyArray(int handle, const std::vector<T>& src, T* buf, int cap, int* outCount)
{
    MR_Lock();
    ModelData* md = MR_Get(handle);
    if (!md || !outCount) { MR_Unlock(); return MR_ERR_BAD_HANDLE; }
    int need = (int)src.size();
    *outCount = need;
    if (!buf) { MR_Unlock(); return MR_OK; }          /* 只查询大小 */
    if (cap < need)      { MR_Unlock(); return MR_ERR_CAP_TOO_SMALL; }
    if (need > 0) memcpy(buf, src.data(), sizeof(T) * need);
    MR_Unlock();
    return MR_OK;
}

MR_API int MR_GetPositions(int handle, float* buf, int cap, int* outCount)
{ ModelData* md = MR_Get(handle); if(!md){ if(outCount)*outCount=0; return MR_ERR_BAD_HANDLE;} return MR_CopyArray(handle, md->positions, buf, cap, outCount); }
MR_API int MR_GetNormals(int handle, float* buf, int cap, int* outCount)
{ ModelData* md = MR_Get(handle); if(!md){ if(outCount)*outCount=0; return MR_ERR_BAD_HANDLE;} return MR_CopyArray(handle, md->normals, buf, cap, outCount); }
MR_API int MR_GetUVs(int handle, float* buf, int cap, int* outCount)
{ ModelData* md = MR_Get(handle); if(!md){ if(outCount)*outCount=0; return MR_ERR_BAD_HANDLE;} return MR_CopyArray(handle, md->uvs, buf, cap, outCount); }
MR_API int MR_GetColors(int handle, float* buf, int cap, int* outCount)
{ ModelData* md = MR_Get(handle); if(!md){ if(outCount)*outCount=0; return MR_ERR_BAD_HANDLE;} return MR_CopyArray(handle, md->colors, buf, cap, outCount); }
MR_API int MR_GetIndices(int handle, int* buf, int cap, int* outCount)
{ ModelData* md = MR_Get(handle); if(!md){ if(outCount)*outCount=0; return MR_ERR_BAD_HANDLE;} return MR_CopyArray(handle, md->indices, buf, cap, outCount); }
MR_API int MR_GetSubMeshes(int handle, MR_SubMesh* buf, int cap, int* outCount)
{
    ModelData* md = MR_Get(handle);
    if (!md) { if (outCount) *outCount = 0; return MR_ERR_BAD_HANDLE; }
    MR_Lock();
    int need = (int)md->subMeshes.size();
    *outCount = need;
    if (!buf) { MR_Unlock(); return MR_OK; }
    if (cap < need) { MR_Unlock(); return MR_ERR_CAP_TOO_SMALL; }
    for (int i = 0; i < need; i++)
    {
        buf[i].materialIndex = md->subMeshes[i].materialIndex;
        buf[i].indexStart    = md->subMeshes[i].indexStart;
        buf[i].indexCount    = md->subMeshes[i].indexCount;
    }
    MR_Unlock();
    return MR_OK;
}
MR_API int MR_GetMaterials(int handle, MR_Material* buf, int cap, int* outCount)
{
    ModelData* md = MR_Get(handle);
    if (!md) { if (outCount) *outCount = 0; return MR_ERR_BAD_HANDLE; }
    MR_Lock();
    int need = (int)md->materials.size();
    *outCount = need;
    if (!buf) { MR_Unlock(); return MR_OK; }
    if (cap < need) { MR_Unlock(); return MR_ERR_CAP_TOO_SMALL; }
    for (int i = 0; i < need; i++)
    {
        MR_MaterialData& s = md->materials[i];
        memset(&buf[i], 0, sizeof(MR_Material));
        strncpy(buf[i].name, s.name.c_str(), sizeof(buf[i].name) - 1);
        memcpy(buf[i].diffuse,  s.diffuse,  sizeof(float) * 4);
        memcpy(buf[i].ambient,  s.ambient,  sizeof(float) * 4);
        memcpy(buf[i].specular, s.specular, sizeof(float) * 4);
        buf[i].shininess = s.shininess;
        buf[i].hasTexture = s.hasTexture ? 1 : 0;
        strncpy(buf[i].texturePath, s.texturePath.c_str(), sizeof(buf[i].texturePath) - 1);
    }
    MR_Unlock();
    return MR_OK;
}

MR_API void MR_FreeModel(int handle)
{
    MR_Lock();
    ModelData* md = MR_Get(handle);
    if (md)
    {
        delete md;
        g_models[handle - 1] = NULL;
    }
    MR_Unlock();
}

MR_API const char* MR_GetLastError(void)
{
    return g_lastError;
}
