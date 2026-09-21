/*
  ModelLoader.cs —— 模型加载器 (新建, 2026)
  在后台线程调用原生 DLL 解析 OBJ/PLY (不阻塞UI/渲染),
  在主线程构建 GameObject (Mesh + Material + Texture)。
  用法: 场景中任意物体挂载本组件, 调用 LoadFile(path)。
*/
using System;
using System.IO;
using System.Threading;
using UnityEngine;

namespace ModelViewer
{
    public class ModelLoader : MonoBehaviour
    {
        /// <summary>加载进度 0..1</summary>
        public float Progress { get; private set; }
        /// <summary>当前阶段描述</summary>
        public string Stage { get; private set; }

        ModelLoader() { Stage = ""; }

        /// <summary>是否正在加载</summary>
        public bool IsLoading { get; private set; }
        /// <summary>最近一次加载的概要信息</summary>
        public NativeModelBridge.ModelInfo LastInfo;

        /// <summary>加载成功回调 (主线程, 参数为模型根物体)</summary>
        public event Action<GameObject> OnLoaded;
        /// <summary>加载失败回调 (主线程, 错误描述)</summary>
        public event Action<string> OnError;

        Thread worker;
        volatile bool quitWorker;

        // 后台线程结果 -> 主线程构建
        class Result
        {
            public string error;
            public NativeModelBridge.ModelInfo info;
            public float[] positions, normals, uvs, colors;
            public int[] indices;
            public NativeModelBridge.SubMeshData[] subMeshes;
            public NativeModelBridge.MaterialData[] materials;
            public string sourcePath;
            public bool hasColors;
        }
        Result pending;   // 线程写入, 主线程读取
        readonly object resultLock = new object();

        void Update()
        {
            lock (resultLock)
            {
                if (pending != null)
                {
                    Result r = pending;
                    pending = null;
                    Finish(r);
                }
            }
        }

        void OnDestroy()
        {
            quitWorker = true;
            if (worker != null && worker.IsAlive) worker.Join(500);
        }

        /// <summary>开始异步加载 OBJ/PLY 文件 (支持中文路径)</summary>
        public void LoadFile(string path)
        {
            if (IsLoading)
            {
                if (OnError != null) OnError("正在加载其他模型, 请稍候");
                return;
            }
            if (string.IsNullOrEmpty(path) || !File.Exists(path))
            {
                if (OnError != null) OnError("文件不存在: " + path);
                return;
            }

            IsLoading = true;
            Progress = 0f;
            Stage = "准备加载...";
            string captured = path;

            worker = new Thread(delegate () { WorkerMain(captured); });
            worker.IsBackground = true;
            worker.Start();
        }

        // ---------------- 后台线程 ----------------
        void WorkerMain(string path)
        {
            Result r = new Result();
            r.sourcePath = path;
            try
            {
                Stage = "解析模型文件...";
                Progress = 0.1f;

                int h = NativeModelBridge.LoadModel(path);
                if (h <= 0)
                {
                    r.error = NativeModelBridge.GetLastError();
                    Deliver(r);
                    return;
                }

                Stage = "读取网格数据...";
                Progress = 0.55f;

                NativeModelBridge.ModelInfo info;
                if (NativeModelBridge.GetModelInfo(h, out info) != NativeModelBridge.OK)
                {
                    r.error = "获取模型信息失败";
                    NativeModelBridge.FreeModel(h);
                    Deliver(r);
                    return;
                }
                r.info = info;
                r.hasColors = info.hasVertexColors != 0;

                int need;
                int got;

                if (NativeModelBridge.GetPositions(h, null, 0, out need) != NativeModelBridge.OK) need = 0;
                r.positions = new float[need];
                if (need > 0) NativeModelBridge.GetPositions(h, r.positions, need, out got);
                Progress = 0.65f;

                if (NativeModelBridge.GetNormals(h, null, 0, out need) != NativeModelBridge.OK) need = 0;
                r.normals = new float[need];
                if (need > 0) NativeModelBridge.GetNormals(h, r.normals, need, out got);

                if (NativeModelBridge.GetUVs(h, null, 0, out need) != NativeModelBridge.OK) need = 0;
                r.uvs = new float[need];
                if (need > 0) NativeModelBridge.GetUVs(h, r.uvs, need, out got);
                Progress = 0.75f;

                if (r.hasColors)
                {
                    if (NativeModelBridge.GetColors(h, null, 0, out need) != NativeModelBridge.OK) need = 0;
                    if (need > 0)
                    {
                        r.colors = new float[need];
                        NativeModelBridge.GetColors(h, r.colors, need, out got);
                    }
                }

                if (NativeModelBridge.GetIndices(h, null, 0, out need) != NativeModelBridge.OK) need = 0;
                r.indices = new int[need];
                if (need > 0) NativeModelBridge.GetIndices(h, r.indices, need, out got);
                Progress = 0.85f;

                if (NativeModelBridge.GetSubMeshes(h, null, 0, out need) != NativeModelBridge.OK) need = 0;
                r.subMeshes = new NativeModelBridge.SubMeshData[need];
                if (need > 0) NativeModelBridge.GetSubMeshes(h, r.subMeshes, need, out got);

                if (NativeModelBridge.GetMaterials(h, null, 0, out need) != NativeModelBridge.OK) need = 0;
                r.materials = new NativeModelBridge.MaterialData[need];
                if (need > 0) NativeModelBridge.GetMaterials(h, r.materials, need, out got);

                // 数据已拷贝到托管内存, 立即释放原生内存(降低峰值占用)
                NativeModelBridge.FreeModel(h);

                if (r.positions == null || r.positions.Length == 0 ||
                    r.indices == null || r.indices.Length == 0)
                {
                    r.error = "模型中没有可用的几何数据";
                    Deliver(r);
                    return;
                }

                Stage = "构建网格...";
                Progress = 0.95f;
                Deliver(r);
            }
            catch (Exception e)
            {
                r.error = "加载异常: " + e.Message;
                Deliver(r);
            }
        }

        void Deliver(Result r)
        {
            lock (resultLock) { pending = r; }
        }

        // ---------------- 主线程: 构建模型 ----------------
        void Finish(Result r)
        {
            IsLoading = false;
            Progress = 1f;
            Stage = "";

            if (r.error != null)
            {
                if (OnError != null) OnError(r.error);
                return;
            }

            int vertCount = r.positions.Length / 3;
            Mesh mesh = new Mesh();
            // 超过65535顶点时启用32位索引
            mesh.indexFormat = vertCount > 65535
                ? UnityEngine.Rendering.IndexFormat.UInt32
                : UnityEngine.Rendering.IndexFormat.UInt16;
            mesh.vertices = ToVector3(r.positions);
            if (r.normals != null && r.normals.Length == r.positions.Length)
                mesh.normals = ToVector3(r.normals);
            if (r.uvs != null && r.uvs.Length == vertCount * 2)
                mesh.uv = ToVector2(r.uvs);
            if (r.colors != null && r.colors.Length == vertCount * 4)
                mesh.colors = ToColor(r.colors);

            mesh.subMeshCount = r.subMeshes.Length;
            for (int i = 0; i < r.subMeshes.Length; i++)
            {
                var sm = r.subMeshes[i];
                int[] slice = new int[sm.indexCount];
                Array.Copy(r.indices, sm.indexStart, slice, 0, sm.indexCount);
                mesh.SetIndices(slice, MeshTopology.Triangles, i);
            }
            mesh.RecalculateBounds();
            if (r.normals == null || r.normals.Length != r.positions.Length)
                mesh.RecalculateNormals();
            mesh.UploadMeshData(false);

            // ---- 材质 ----
            Shader vcolShader = Shader.Find("Custom/ModelVertexColor");
            Shader stdShader = Shader.Find("Standard");
            if (stdShader == null) stdShader = Shader.Find("Legacy Shaders/Diffuse");

            Material[] mats = new Material[Math.Max(1, r.materials.Length)];
            for (int i = 0; i < mats.Length; i++)
            {
                NativeModelBridge.MaterialData md = i < r.materials.Length ? r.materials[i]
                    : new NativeModelBridge.MaterialData();
                bool useVcol = r.hasColors && vcolShader != null;
                Shader shader = useVcol ? vcolShader : stdShader;
                Material m = new Material(shader);
                m.name = string.IsNullOrEmpty(md.name) ? ("mat_" + i) : md.name;

                Color c = Color.white;
                if (md.diffuse != null && md.diffuse.Length >= 4)
                    c = new Color(md.diffuse[0], md.diffuse[1], md.diffuse[2], 1f);
                m.color = c;
                if (stdShader != null && shader == stdShader)
                {
                    m.SetFloat("_Glossiness", Mathf.Clamp01(md.shininess / 128f));
                    if (md.specular != null && md.specular.Length >= 4)
                        m.SetColor("_SpecColor", new Color(md.specular[0], md.specular[1], md.specular[2], 1f));
                }

                // 纹理 (来自 MTL 的 map_Kd, 缺失时保留纯色并提示)
                if (md.hasTexture == 1 && !string.IsNullOrEmpty(md.texturePath))
                {
                    try
                    {
                        if (File.Exists(md.texturePath))
                        {
                            Texture2D tex = new Texture2D(2, 2, TextureFormat.RGBA32, false);
                            tex.LoadImage(File.ReadAllBytes(md.texturePath), false);
                            tex.name = Path.GetFileNameWithoutExtension(md.texturePath);
                            m.mainTexture = tex;
                        }
                        else if (OnError != null)
                        {
                            OnError("提示: 材质 \"" + m.name + "\" 的纹理缺失(" + Path.GetFileName(md.texturePath) + "), 已使用纯色");
                        }
                    }
                    catch (Exception e)
                    {
                        if (OnError != null) OnError("提示: 纹理加载失败(" + e.Message + "), 已使用纯色");
                    }
                }
                mats[i] = m;
            }

            // ---- 场景物体 ----
            GameObject root = new GameObject("MODEL_" + r.info.name);
            MeshFilter mf = root.AddComponent<MeshFilter>();
            mf.sharedMesh = mesh;
            MeshRenderer mr = root.AddComponent<MeshRenderer>();
            mr.sharedMaterials = mats;

            LastInfo = r.info;
            if (OnLoaded != null) OnLoaded(root);
        }

        // ---- float[] -> Unity 数组转换 ----
        static Vector3[] ToVector3(float[] src)
        {
            Vector3[] dst = new Vector3[src.Length / 3];
            for (int i = 0; i < dst.Length; i++)
                dst[i] = new Vector3(src[3 * i], src[3 * i + 1], src[3 * i + 2]);
            return dst;
        }
        static Vector2[] ToVector2(float[] src)
        {
            Vector2[] dst = new Vector2[src.Length / 2];
            for (int i = 0; i < dst.Length; i++)
                dst[i] = new Vector2(src[2 * i], src[2 * i + 1]);
            return dst;
        }
        static Color[] ToColor(float[] src)
        {
            Color[] dst = new Color[src.Length / 4];
            for (int i = 0; i < dst.Length; i++)
                dst[i] = new Color(src[4 * i], src[4 * i + 1], src[4 * i + 2], src[4 * i + 3]);
            return dst;
        }
    }
}
