/*
  NativeModelBridge.cs —— ModelReader.dll 的 C# 互操作层 (新建, 2026)
  对应原生接口 ModelBridge.h, 支持中文路径 (UTF-8 编码传递)。
*/
using System;
using System.Runtime.InteropServices;
using System.Text;

namespace ModelViewer
{
    public static class NativeModelBridge
    {
        const string Dll = "ModelReader";

        // ---- 错误码 (与 ModelBridge.h 一致) ----
        public const int OK = 0;
        public const int ERR_NULL_PATH = -1;
        public const int ERR_FILE_NOT_FOUND = -2;
        public const int ERR_UNKNOWN_FORMAT = -3;
        public const int ERR_PARSE_FAILED = -4;
        public const int ERR_EMPTY_MESH = -5;
        public const int ERR_BAD_HANDLE = -6;
        public const int ERR_NO_MEMORY = -7;
        public const int ERR_CAP_TOO_SMALL = -8;

        [StructLayout(LayoutKind.Sequential, CharSet = CharSet.Ansi)]
        public struct ModelInfo
        {
            public int vertexCount;      // 源文件顶点数
            public int triangleCount;    // 三角形数
            public int subMeshCount;     // 子网格数
            public int materialCount;    // 材质数
            public int meshVertexCount;  // 展开后网格顶点数
            public int hasVertexColors;  // 是否带顶点颜色
            [MarshalAs(UnmanagedType.ByValTStr, SizeConst = 16)]  public string format;    // "OBJ"/"PLY ASCII"/...
            [MarshalAs(UnmanagedType.ByValTStr, SizeConst = 256)] public string name;      // 文件名
            public double fileSizeBytes;
            public double loadTimeSeconds;
        }

        [StructLayout(LayoutKind.Sequential)]
        public struct SubMeshData
        {
            public int materialIndex;
            public int indexStart;
            public int indexCount;
        }

        [StructLayout(LayoutKind.Sequential, CharSet = CharSet.Ansi)]
        public struct MaterialData
        {
            [MarshalAs(UnmanagedType.ByValTStr, SizeConst = 128)] public string name;
            [MarshalAs(UnmanagedType.ByValArray, SizeConst = 4)] public float[] diffuse;
            [MarshalAs(UnmanagedType.ByValArray, SizeConst = 4)] public float[] ambient;
            [MarshalAs(UnmanagedType.ByValArray, SizeConst = 4)] public float[] specular;
            public float shininess;
            public int hasTexture;
            [MarshalAs(UnmanagedType.ByValTStr, SizeConst = 1024)] public string texturePath;
        }

        // ---- 基础 P/Invoke ----
        [DllImport(Dll)] static extern int  MR_LoadModel(byte[] utf8Path);
        [DllImport(Dll)] static extern int  MR_GetModelInfo(int handle, out ModelInfo outInfo);
        [DllImport(Dll)] static extern int  MR_GetPositions(int handle, float[] buf, int cap, out int outCount);
        [DllImport(Dll)] static extern int  MR_GetNormals(int handle, float[] buf, int cap, out int outCount);
        [DllImport(Dll)] static extern int  MR_GetUVs(int handle, float[] buf, int cap, out int outCount);
        [DllImport(Dll)] static extern int  MR_GetColors(int handle, float[] buf, int cap, out int outCount);
        [DllImport(Dll)] static extern int  MR_GetIndices(int handle, int[] buf, int cap, out int outCount);
        [DllImport(Dll)] static extern int  MR_GetSubMeshes(int handle, [Out] SubMeshData[] buf, int cap, out int outCount);
        [DllImport(Dll)] static extern int  MR_GetMaterials(int handle, [Out] MaterialData[] buf, int cap, out int outCount);
        [DllImport(Dll)] static extern void MR_FreeModel(int handle);
        [DllImport(Dll)] static extern IntPtr MR_GetLastError();

        /// <summary>读取最近一次错误描述 (UTF-8)</summary>
        public static string GetLastError()
        {
            IntPtr p = MR_GetLastError();
            if (p == IntPtr.Zero) return "";
            int len = 0;
            while (Marshal.ReadByte(p, len) != 0) len++;
            byte[] b = new byte[len];
            Marshal.Copy(p, b, 0, len);
            return Encoding.UTF8.GetString(b);
        }

        /// <summary>加载模型, 返回句柄(>0)或负错误码</summary>
        public static int LoadModel(string utf8Path)
        {
            byte[] bytes = Encoding.UTF8.GetBytes(utf8Path + "\0");
            return MR_LoadModel(bytes);
        }

        public static int GetModelInfo(int handle, out ModelInfo info) { return MR_GetModelInfo(handle, out info); }
        public static int GetPositions(int h, float[] b, int cap, out int n) { return MR_GetPositions(h, b, cap, out n); }
        public static int GetNormals(int h, float[] b, int cap, out int n) { return MR_GetNormals(h, b, cap, out n); }
        public static int GetUVs(int h, float[] b, int cap, out int n) { return MR_GetUVs(h, b, cap, out n); }
        public static int GetColors(int h, float[] b, int cap, out int n) { return MR_GetColors(h, b, cap, out n); }
        public static int GetIndices(int h, int[] b, int cap, out int n) { return MR_GetIndices(h, b, cap, out n); }
        public static int GetSubMeshes(int h, SubMeshData[] b, int cap, out int n) { return MR_GetSubMeshes(h, b, cap, out n); }
        public static int GetMaterials(int h, MaterialData[] b, int cap, out int n) { return MR_GetMaterials(h, b, cap, out n); }
        public static void FreeModel(int handle) { MR_FreeModel(handle); }
    }
}
