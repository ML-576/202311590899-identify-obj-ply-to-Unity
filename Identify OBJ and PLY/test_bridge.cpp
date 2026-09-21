/*
  test_bridge.cpp —— ModelReader.dll 控制台测试程序 (新建文件, 2026)
  用法: test_bridge.exe 文件1 [文件2 ...]
  支持 UTF-16 命令行参数(中文路径), 内部转为 UTF-8 传给插件。
*/
#include "pch.h"
#include "ModelBridge.h"
#include <string>
#include <vector>

#ifdef _WIN32
#include <windows.h>
static std::string ToUtf8(const wchar_t* w)
{
    int n = WideCharToMultiByte(CP_UTF8, 0, w, -1, NULL, 0, NULL, NULL);
    std::string s(n > 0 ? n - 1 : 0, 0);
    if (n > 1) WideCharToMultiByte(CP_UTF8, 0, w, -1, &s[0], n, NULL, NULL);
    return s;
}
#else
#include <stdlib.h>
static std::string ToUtf8(const char* a) { return std::string(a ? a : ""); }
#define main_run main
#endif

#ifdef _WIN32
int wmain(int argc, wchar_t** argv)
#else
int main(int argc, char** argv)
#endif
{
    if (argc < 2)
    {
        printf("用法: test_bridge.exe <模型文件.obj/.ply> [更多文件...]\n");
        return 1;
    }

    int failCount = 0;
    for (int a = 1; a < argc; a++)
    {
        std::string path =
#ifdef _WIN32
            ToUtf8(argv[a]);
#else
            ToUtf8(argv[a]);
#endif
        printf("\n=============== 加载: %s ===============\n", path.c_str());

        int h = MR_LoadModel(path.c_str());
        if (h <= 0)
        {
            printf("  [失败] 错误码=%d, 描述: %s\n", h, MR_GetLastError());
            failCount++;
            continue;
        }

        MR_ModelInfo info;
        if (MR_GetModelInfo(h, &info) != MR_OK)
        {
            printf("  [失败] 获取信息失败\n");
            failCount++;
            continue;
        }
        printf("  文件名     : %s\n", info.name);
        printf("  格式       : %s\n", info.format);
        printf("  源顶点数   : %d\n", info.vertexCount);
        printf("  三角形数   : %d\n", info.triangleCount);
        printf("  网格顶点数 : %d\n", info.meshVertexCount);
        printf("  子网格数   : %d\n", info.subMeshCount);
        printf("  材质数     : %d\n", info.materialCount);
        printf("  顶点颜色   : %s\n", info.hasVertexColors ? "有" : "无");
        printf("  文件大小   : %.1f KB\n", info.fileSizeBytes / 1024.0);
        printf("  加载耗时   : %.3f 秒\n", info.loadTimeSeconds);

        /* 取前3个顶点和法线验证数据 */
        int needP = 0;
        MR_GetPositions(h, NULL, 0, &needP);
        std::vector<float> pos(needP), nrm(needP), uv(needP * 2 / 3 * 3);
        int gotP = 0;
        MR_GetPositions(h, pos.data(), needP, &gotP);
        MR_GetNormals(h, nrm.data(), needP, &gotP);
        MR_GetUVs(h, uv.data(), (int)uv.size(), &gotP);
        for (int i = 0; i < (int)pos.size() / 3 && i < 3; i++)
            printf("  v[%d] = (%.3f, %.3f, %.3f)  n=(%.2f,%.2f,%.2f) uv=(%.2f,%.2f)\n",
                   i, pos[3*i], pos[3*i+1], pos[3*i+2],
                   nrm[3*i], nrm[3*i+1], nrm[3*i+2], uv[2*i], uv[2*i+1]);

        /* 材质信息 */
        int needM = 0;
        MR_GetMaterials(h, NULL, 0, &needM);
        std::vector<MR_Material> mats(needM);
        int gotM = 0;
        MR_GetMaterials(h, mats.data(), needM, &gotM);
        for (int i = 0; i < gotM; i++)
            printf("  材质[%d] \"%s\"  Kd=(%.2f,%.2f,%.2f)  纹理=%s\n",
                   i, mats[i].name, mats[i].diffuse[0], mats[i].diffuse[1],
                   mats[i].diffuse[2], mats[i].hasTexture ? mats[i].texturePath : "(无)");

        /* 子网格与索引 */
        int needS = 0;
        MR_GetSubMeshes(h, NULL, 0, &needS);
        std::vector<MR_SubMesh> subs(needS);
        int gotS = 0;
        MR_GetSubMeshes(h, subs.data(), needS, &gotS);
        int needI = 0;
        MR_GetIndices(h, NULL, 0, &needI);
        printf("  子网格: %d 个, 索引总数: %d\n", gotS, needI);
        for (int i = 0; i < gotS; i++)
            printf("    sub[%d] mat=%d start=%d count=%d\n",
                   i, subs[i].materialIndex, subs[i].indexStart, subs[i].indexCount);

        /* 顶点颜色(如有) */
        if (info.hasVertexColors)
        {
            int needC = 0;
            MR_GetColors(h, NULL, 0, &needC);
            std::vector<float> col(needC);
            int gotC = 0;
            MR_GetColors(h, col.data(), needC, &gotC);
            for (int i = 0; i < gotC / 4 && i < 3; i++)
                printf("  color[%d] = (%.2f, %.2f, %.2f, %.2f)\n",
                       i, col[4*i], col[4*i+1], col[4*i+2], col[4*i+3]);
        }

        MR_FreeModel(h);
        printf("  [成功] 加载并释放完成\n");
    }

    printf("\n测试完成, 失败 %d 个\n", failCount);
    return failCount;
}
