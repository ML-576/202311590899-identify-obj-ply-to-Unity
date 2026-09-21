/*
  ModelViewerUI.cs —— 模型浏览器主界面 (新建, 2026)
  功能: 文件选择(内置跨平台文件浏览器) / 加载控制 / 进度反馈 /
        模型信息展示 / 操作说明 / 错误提示 / 自动旋转
  挂载: 场景中任意物体。首次运行会自动创建相机(带OrbitCamera)与平行光。
*/
using System;
using System.IO;
using UnityEngine;

namespace ModelViewer
{
    public class ModelViewerUI : MonoBehaviour
    {
        ModelLoader loader;
        OrbitCamera orbit;
        GameObject currentModel;

        // ---- UI 状态 ----
        bool showBrowser = false;
        bool showInfo = true;
        bool showHelp = false;
        bool autoRotate = false;
        Vector2 infoScroll, helpScroll;
        string lastError = "";
        bool showError = false;

        // ---- 文件浏览器状态 ----
        string browseDir;
        string browseFilter = "";
        string[] dirEntries = new string[0];
        bool[] entryIsDir = new bool[0];

        Rect winRect = new Rect(20, 60, 460, 380);

        void Awake()
        {
            // ---- 确保场景有相机与光照 ----
            Camera cam = Camera.main;
            if (cam == null)
            {
                var go = new GameObject("MainCamera");
                go.tag = "MainCamera";
                cam = go.AddComponent<Camera>();
                cam.backgroundColor = new Color(0.16f, 0.18f, 0.22f);
                cam.clearFlags = CameraClearFlags.SolidColor;
                go.AddComponent<AudioListener>();
            }
            orbit = cam.GetComponent<OrbitCamera>();
            if (orbit == null) orbit = cam.gameObject.AddComponent<OrbitCamera>();

            Light[] lights = FindObjectsOfType<Light>();
            bool hasLight = false;
            foreach (var l in lights) if (l.type == LightType.Directional) { hasLight = true; break; }
            if (!hasLight)
            {
                var lgo = new GameObject("DirectionalLight");
                Light li = lgo.AddComponent<Light>();
                li.type = LightType.Directional;
                li.intensity = 1.05f;
                lgo.transform.rotation = Quaternion.Euler(50f, -30f, 0f);
            }

            loader = GetComponent<ModelLoader>();
            if (loader == null) loader = gameObject.AddComponent<ModelLoader>();
            loader.OnLoaded += HandleLoaded;
            loader.OnError += HandleError;

            browseDir = Path.Combine(Application.streamingAssetsPath, "Models");
            if (!Directory.Exists(browseDir)) browseDir = Application.streamingAssetsPath;
            RefreshDir();
        }

        void OnDestroy()
        {
            if (loader != null)
            {
                loader.OnLoaded -= HandleLoaded;
                loader.OnError -= HandleError;
            }
        }

        void HandleLoaded(GameObject root)
        {
            if (currentModel != null) Destroy(currentModel);
            currentModel = root;
            orbit.FrameBounds(root.GetComponentInChildren<Renderer>().bounds);
            showBrowser = false;
        }

        void HandleError(string msg)
        {
            lastError = msg;
            showError = true;
        }

        void Update()
        {
            if (Input.GetKeyDown(KeyCode.F) && currentModel != null)
                orbit.FrameGameObject(currentModel);
            if (Input.GetKeyDown(KeyCode.R) && currentModel != null)
                orbit.FrameGameObject(currentModel);

            if (autoRotate && currentModel != null && !loader.IsLoading)
                currentModel.transform.Rotate(Vector3.up, 30f * Time.deltaTime, Space.World);
        }

        // ================= IMGUI =================
        void OnGUI()
        {
            DrawToolbar();

            if (showBrowser)
            {
                winRect = GUILayout.Window(100, winRect, DrawBrowserWindow, "选择模型文件 (OBJ / PLY)");
            }

            if (showInfo && currentModel != null) DrawInfoPanel();
            if (showHelp) DrawHelpPanel();
            if (loader.IsLoading) DrawProgress();
            if (showError) DrawErrorDialog();
        }

        void DrawToolbar()
        {
            GUILayout.BeginHorizontal(GetStyleToolbar());
            GUILayout.Label("三维模型浏览器", GUILayout.Width(110));
            if (GUILayout.Button("加载模型", GUILayout.Width(80))) { showBrowser = true; RefreshDir(); }
            if (GUILayout.Button("重置视角", GUILayout.Width(80)))
            {
                if (currentModel != null) orbit.FrameGameObject(currentModel);
                else { orbit.focusPoint = Vector3.zero; orbit.distance = 8f; orbit.yaw = 45f; orbit.pitch = 30f; }
            }
            autoRotate = GUILayout.Toggle(autoRotate, "自动旋转", GUILayout.Width(74));
            showInfo = GUILayout.Toggle(showInfo, "模型信息", GUILayout.Width(74));
            showHelp = GUILayout.Toggle(showHelp, "操作说明", GUILayout.Width(74));
            GUILayout.FlexibleSpace();
            GUILayout.EndHorizontal();
        }

        GUIStyle GetStyleToolbar()
        {
            GUIStyle s = new GUIStyle(GUI.skin.box);
            s.alignment = TextAnchor.MiddleLeft;
            return s;
        }

        // ---------- 文件浏览器 ----------
        void RefreshDir()
        {
            try
            {
                if (string.IsNullOrEmpty(browseDir)) browseDir = "/";
                if (Directory.Exists(browseDir))
                {
                    var dirs = Directory.GetDirectories(browseDir);
                    var files = Directory.GetFiles(browseDir);
                    var list = new System.Collections.Generic.List<string>();
                    var flags = new System.Collections.Generic.List<bool>();
                    foreach (var d in dirs) { list.Add(Path.GetFileName(d)); flags.Add(true); }
                    foreach (var f in files)
                    {
                        string ext = Path.GetExtension(f).ToLower();
                        if (ext == ".obj" || ext == ".ply") { list.Add(Path.GetFileName(f)); flags.Add(false); }
                    }
                    dirEntries = list.ToArray();
                    entryIsDir = flags.ToArray();
                }
                else dirEntries = new string[0];
            }
            catch (Exception e) { dirEntries = new string[0]; lastError = "无法读取目录: " + e.Message; showError = true; }
        }

        void DrawBrowserWindow(int id)
        {
            GUILayout.BeginHorizontal();
            GUILayout.Label("路径: " + browseDir, GUI.skin.label);
            GUILayout.EndHorizontal();
            GUILayout.BeginHorizontal();
            GUILayout.Label("过滤:", GUILayout.Width(36));
            string nf = GUILayout.TextField(browseFilter);
            if (nf != browseFilter) { browseFilter = nf; }
            if (GUILayout.Button("上一级", GUILayout.Width(60))) { DirectoryInfo di = Directory.GetParent(browseDir); browseDir = di != null ? di.FullName : browseDir; RefreshDir(); }
            if (GUILayout.Button("刷新", GUILayout.Width(46))) RefreshDir();
            GUILayout.EndHorizontal();

            infoScroll = GUILayout.BeginScrollView(infoScroll);
            for (int i = 0; i < dirEntries.Length; i++)
            {
                string name = dirEntries[i];
                if (!string.IsNullOrEmpty(browseFilter) && entryIsDir[i] == false &&
                    !name.ToLower().Contains(browseFilter.ToLower())) continue;

                Rect r = GUILayoutUtility.GetRect(0, 22, GUILayout.ExpandWidth(true));
                bool hover = r.Contains(Event.current.mousePosition);
                if (hover) GUI.DrawTexture(r, Texture2D.whiteTexture, ScaleMode.StretchToFill, true, 0, new Color(1, 1, 1, 0.08f), 0, 0);
                string icon = entryIsDir[i] ? "[文件夹] " : "[模型]   ";
                GUI.Label(r, icon + name);

                if (Event.current.type == EventType.MouseDown && r.Contains(Event.current.mousePosition))
                {
                    string full = Path.Combine(browseDir, name);
                    if (entryIsDir[i])
                    {
                        browseDir = full; RefreshDir();
                        Event.current.Use();
                    }
                    else
                    {
                        Event.current.Use();
                        showBrowser = false;
                        loader.LoadFile(full);
                    }
                }
            }
            GUILayout.EndScrollView();
        }

        // ---------- 信息面板 ----------
        void DrawInfoPanel()
        {
            var i = loader.LastInfo;
            string text =
                "文件: " + i.name + "\n" +
                "格式: " + i.format + "\n" +
                "文件大小: " + FormatSize(i.fileSizeBytes) + "\n" +
                "顶点数量(源): " + i.vertexCount + "\n" +
                "三角形数量: " + i.triangleCount + "\n" +
                "网格顶点数(渲染): " + i.meshVertexCount + "\n" +
                "子网格(组)数量: " + i.subMeshCount + "\n" +
                "材质数量: " + i.materialCount + "\n" +
                "顶点颜色: " + (i.hasVertexColors != 0 ? "有" : "无") + "\n" +
                "加载耗时: " + i.loadTimeSeconds.ToString("F3") + " 秒";
            GUILayout.BeginArea(new Rect(20, 100, 240, 320), text, GUI.skin.box);
            GUILayout.EndArea();
        }

        // ---------- 帮助面板 ----------
        void DrawHelpPanel()
        {
            string text =
                "【浏览操作】\n" +
                "  旋转视角: 按住鼠标右键拖拽 (或 Alt+左键)\n" +
                "  平移视角: 按住鼠标中键拖拽 (或 右键+Shift)\n" +
                "  缩放: 鼠标滚轮\n" +
                "  聚焦模型: F 键 / R 键\n\n" +
                "【加载模型】\n" +
                "  1. 点击\"加载模型\"打开文件浏览器\n" +
                "  2. 双击进入文件夹, 点击 .obj 或 .ply 文件加载\n" +
                "  3. 支持中文路径; PLY 支持ASCII与二进制(大/小端)\n\n" +
                "【说明】\n" +
                "  - 大模型采用后台线程加载, 界面不卡顿\n" +
                "  - OBJ 自动解析 MTL 材质与贴图(map_Kd)\n" +
                "  - PLY 自动读取顶点颜色与法线";
            GUILayout.BeginArea(new Rect(Screen.width - 360, 100, 340, 400), "", GUI.skin.box);
            helpScroll = GUILayout.BeginScrollView(helpScroll);
            GUILayout.Label(text);
            GUILayout.EndScrollView();
            GUILayout.EndArea();
        }

        // ---------- 进度显示 ----------
        void DrawProgress()
        {
            float w = 320, h = 74;
            Rect r = new Rect((Screen.width - w) / 2, (Screen.height - h) / 2, w, h);
            GUI.Box(r, "正在加载模型");
            Rect bar = new Rect(r.x + 16, r.y + 34, w - 32, 18);
            GUI.DrawTexture(new Rect(bar.x, bar.y, bar.width * Mathf.Clamp01(loader.Progress), bar.height),
                            Texture2D.whiteTexture, ScaleMode.StretchToFill, true, 0, new Color(0.3f, 0.75f, 0.4f), 0, 3);
            GUI.Label(new Rect(r.x + 16, r.y + 52, w - 32, 20), loader.Stage);
        }

        // ---------- 错误对话框 ----------
        void DrawErrorDialog()
        {
            float w = 380, h = 130;
            Rect r = new Rect((Screen.width - w) / 2, (Screen.height - h) / 2, w, h);
            GUI.Box(r, "提示");
            GUILayout.BeginArea(new Rect(r.x + 12, r.y + 26, w - 24, h - 70));
            GUIStyle wrapStyle = new GUIStyle(GUI.skin.label);
            wrapStyle.wordWrap = true;
            GUILayout.Label(lastError, wrapStyle);
            GUILayout.EndArea();
            if (GUI.Button(new Rect(r.x + w / 2 - 40, r.yMax - 38, 80, 26), "确定"))
                showError = false;
        }

        static string FormatSize(double bytes)
        {
            if (bytes > 1024 * 1024) return (bytes / 1024 / 1024).ToString("F1") + " MB";
            return (bytes / 1024).ToString("F1") + " KB";
        }
    }
}
