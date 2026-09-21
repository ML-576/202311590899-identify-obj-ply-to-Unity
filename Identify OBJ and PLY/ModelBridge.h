/*
  ModelBridge.h —— Unity 原生插件 C 接口 (新建文件, 2026)
  ------------------------------------------------------------------
  作用: 将本目录提供的 OBJ 解析模块(OBJ/glm) 与 PLY 解析模块
  (hply/ReadPly/RawMesh) 封装为扁平的 C 接口, 供 Unity C#
  通过 [DllImport] 调用。解析结果以"每顶点展开"的形式输出:
      positions / normals / uvs : float 数组 (3|3|2 per vertex)
      colors                    : float 数组 (4 per vertex, 可能为空)
      indices                   : int 数组 (3 per triangle, 32位索引)
      subMeshes                 : 按组划分的索引区间(用于多材质)
  ------------------------------------------------------------------
  两段式调用流程 (Unity 侧):
      1) MR_LoadModel(utf8Path)                    -> 返回句柄(>0)或负错误码
      2) MR_GetModelInfo(handle, &info)            -> 查询数量
      3) MR_GetPositions(handle, NULL, 0, &need)   -> 查询所需缓冲大小
         MR_GetPositions(handle, buf, cap, &got)   -> 填充数据
         (Normals/UVs/Colors/Indices/SubMeshes/Materials 同上)
      4) MR_FreeModel(handle)                      -> 释放
*/
#ifndef MODEL_BRIDGE_H_
#define MODEL_BRIDGE_H_

#ifdef _WIN32
  #ifdef MODEL_BRIDGE_EXPORTS
    #define MR_API __declspec(dllexport)
  #else
    #define MR_API __declspec(dllimport)
  #endif
#else
  #define MR_API __attribute__((visibility("default")))
#endif

#ifdef __cplusplus
extern "C" {
#endif

/* ---- 错误码 (MR_LoadModel 返回负值, 各Get函数返回0/-1) ---- */
#define MR_OK                  0
#define MR_ERR_NULL_PATH      (-1)   /* 路径为空 */
#define MR_ERR_FILE_NOT_FOUND (-2)   /* 文件不存在 */
#define MR_ERR_UNKNOWN_FORMAT (-3)   /* 不是 .obj/.ply 文件 */
#define MR_ERR_PARSE_FAILED   (-4)   /* 解析失败(格式错误/文件损坏) */
#define MR_ERR_EMPTY_MESH     (-5)   /* 文件中没有可用的几何数据 */
#define MR_ERR_BAD_HANDLE     (-6)   /* 句柄无效 */
#define MR_ERR_NO_MEMORY      (-7)   /* 内存不足 */
#define MR_ERR_CAP_TOO_SMALL  (-8)   /* 调用方缓冲区过小 */

/* 模型概要信息 */
typedef struct MR_ModelInfo
{
    int    vertexCount;      /* 源文件顶点数 */
    int    triangleCount;    /* 三角形数 */
    int    subMeshCount;     /* 子网格数(OBJ按组, PLY为1) */
    int    materialCount;    /* 材质数 */
    int    meshVertexCount;  /* 展开后网格顶点数(positions.size()/3) */
    int    hasVertexColors;  /* 是否带顶点颜色(PLY) */
    char   format[16];       /* "OBJ" / "PLY ASCII" / "PLY BIN LE" / "PLY BIN BE" */
    char   name[256];        /* 文件名(UTF-8) */
    double fileSizeBytes;    /* 文件大小(字节) */
    double loadTimeSeconds;  /* 解析耗时(秒) */
} MR_ModelInfo;

/* 子网格: 引用 indices 的一个区间 */
typedef struct MR_SubMesh
{
    int materialIndex;   /* 使用的材质索引 */
    int indexStart;      /* 在indices中的起始位置 */
    int indexCount;      /* 索引数量(必为3的倍数) */
} MR_SubMesh;

/* 材质 (颜色为0..1的RGBA, 纹理为绝对路径或空) */
typedef struct MR_Material
{
    char  name[128];
    float diffuse[4];
    float ambient[4];
    float specular[4];
    float shininess;       /* 0..128 */
    int   hasTexture;      /* 1=texturePath有效 */
    char  texturePath[1024]; /* 纹理图片绝对路径(UTF-8), 来自MTL的map_Kd */
} MR_Material;

/* 加载模型。utf8Path 为 UTF-8 编码路径(支持中文)。
   成功返回句柄(>0), 失败返回负错误码。 */
MR_API int  MR_LoadModel(const char* utf8Path);

/* 查询模型概要信息, 返回 MR_OK 或 MR_ERR_BAD_HANDLE */
MR_API int  MR_GetModelInfo(int handle, MR_ModelInfo* outInfo);

/* 数据获取。buf 为 NULL 时只查询所需元素个数(写入 *outCount);
   否则 cap 为缓冲区容量(元素个数), 成功返回 MR_OK。
   元素个数: Positions/Normals = meshVertexCount*3, UVs = meshVertexCount*2,
             Colors = meshVertexCount*4(或0), Indices = 3*triangleCount,
             SubMeshes = subMeshCount, Materials = materialCount */
MR_API int  MR_GetPositions(int handle, float* buf, int cap, int* outCount);
MR_API int  MR_GetNormals  (int handle, float* buf, int cap, int* outCount);
MR_API int  MR_GetUVs      (int handle, float* buf, int cap, int* outCount);
MR_API int  MR_GetColors   (int handle, float* buf, int cap, int* outCount);
MR_API int  MR_GetIndices  (int handle, int*   buf, int cap, int* outCount);
MR_API int  MR_GetSubMeshes(int handle, MR_SubMesh*  buf, int cap, int* outCount);
MR_API int  MR_GetMaterials(int handle, MR_Material* buf, int cap, int* outCount);

/* 释放模型 */
MR_API void MR_FreeModel(int handle);

/* 最近一次错误描述 (UTF-8 字符串) */
MR_API const char* MR_GetLastError(void);

#ifdef __cplusplus
}
#endif

#endif /* MODEL_BRIDGE_H_ */
