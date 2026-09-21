// UnityStubs.cs —— 仅用于 csc 语法检查的 UnityEngine 桩 (不属于交付内容)
using System;
namespace UnityEngine
{
    public struct Vector2 { public float x, y; public Vector2(float x, float y) { this.x = x; this.y = y; }
        public static Vector2 zero = new Vector2(0, 0); }
    public struct Vector3 { public float x, y, z; public Vector3(float x, float y, float z) { this.x = x; this.y = y; this.z = z; }
        public float magnitude { get { return (float)Math.Sqrt(x * x + y * y + z * z); } }
        public static Vector3 up = new Vector3(0, 1, 0); public static Vector3 zero = new Vector3(0, 0, 0);
        public static Vector3 operator *(Vector3 v, float s) { return new Vector3(v.x * s, v.y * s, v.z * s); }
        public static Vector3 operator +(Vector3 a, Vector3 b) { return new Vector3(a.x + b.x, a.y + b.y, a.z + b.z); } }
    public struct Quaternion { public static Quaternion Euler(float x, float y, float z) { return new Quaternion(); }
        public static Vector3 operator *(Quaternion q, Vector3 v) { return v; } }
    public struct Color { public float r, g, b, a; public Color(float r, float g, float b, float a) { this.r = r; this.g = g; this.b = b; this.a = a; }
        public Color(float r, float g, float b) : this(r, g, b, 1f) { }
        public static Color white = new Color(1, 1, 1, 1); }
    public struct Rect { public float x, y, width, height; public Rect(float x, float y, float w, float h) { this.x = x; this.y = y; width = w; height = h; }
        public float xMax { get { return x + width; } } public float yMax { get { return y + height; } } public bool Contains(Vector2 p) { return false; } }
    public struct Bounds { public Vector3 center; public Vector3 extents; }
    public enum LightType { Directional, Point, Spot }
    public enum CameraClearFlags { Skybox, SolidColor, Depth, Nothing }
    public enum TextureFormat { RGBA32 }
    public enum Space { World }
    public enum KeyCode { F, R, LeftAlt, LeftShift }
    public enum EventType { MouseDown, MouseDrag, MouseUp }
    public enum ScaleMode { StretchToFill }
    public enum TextAnchor { MiddleLeft }
    public enum FilterMode { Point }
    public enum MeshTopology { Triangles }

    public class Object { public string name;
        public static void Destroy(Object o) { }
        public static T[] FindObjectsOfType<T>() where T : Object { return new T[0]; } }

    public class Transform { public Vector3 position; public Quaternion rotation;
        public Vector3 right = new Vector3(1, 0, 0); public Vector3 up = new Vector3(0, 1, 0);
        public void Rotate(Vector3 axis, float angle, Space s) { } public void LookAt(Vector3 p) { } }

    public class GameObject : Object {
        public GameObject(string n = null) { name = n; }
        public string tag; public Transform transform = new Transform();
        public T AddComponent<T>() where T : Component { T t = (T)Activator.CreateInstance(typeof(T), true); t.gameObject = this; return t; }
        public T GetComponent<T>() where T : Component { return null; }
        public T GetComponentInChildren<T>() where T : Component { return null; } }

    public class Component : Object { public Transform transform = new Transform(); public GameObject gameObject;
        public T GetComponent<T>() where T : Component { return null; } }

    public class MonoBehaviour : Component { public void Awake() { } }
    public class Camera : Component { public static Camera main; public Color backgroundColor; public CameraClearFlags clearFlags; public float fieldOfView = 60; }
    public class AudioListener : Component { }
    public class Light : Component { public LightType type; public float intensity; }
    public class Renderer : Component { public Bounds bounds; }

    namespace Rendering { public enum IndexFormat { UInt16, UInt32 } }

    public class Mesh : Object {
        public UnityEngine.Rendering.IndexFormat indexFormat;
        public Vector3[] vertices; public Vector3[] normals; public Vector2[] uv; public Color[] colors;
        public int subMeshCount;
        public void SetIndices(int[] idx, MeshTopology t, int i) { }
        public void RecalculateBounds() { } public void RecalculateNormals() { } public void UploadMeshData(bool b) { } }
    public class MeshFilter : Component { public Mesh sharedMesh; }
    public class MeshRenderer : Component { public Material sharedMaterial; public Material[] sharedMaterials; }
    public class Material : Object { public Material(Shader s) { } public Color color; public Texture mainTexture;
        public void SetFloat(string k, float v) { } public void SetColor(string k, Color c) { } }
    public class Texture : Object { }
    public class Texture2D : Texture { public Texture2D(int w, int h, TextureFormat f, bool mipmap) { }
        public static Texture2D whiteTexture; public bool LoadImage(byte[] data, bool markNonReadable) { return true; } }
    public class Shader : Object { public static Shader Find(string n) { return null; } }

    public static class Mathf {
        public static float Clamp(float v, float a, float b) { return v < a ? a : v > b ? b : v; }
        public static float Clamp01(float v) { return v < 0 ? 0 : v > 1 ? 1 : v; }
        public static float Abs(float v) { return v < 0 ? -v : v; }
        public static float Tan(float v) { return (float)Math.Tan(v); }
        public static float Sin(float v) { return (float)Math.Sin(v); }
        public static float Cos(float v) { return (float)Math.Cos(v); }
        public static float Pow(float a, float b) { return (float)Math.Pow(a, b); }
        public static float Max(float a, float b) { return a > b ? a : b; }
        public static float PI = 3.14159f; public static float Deg2Rad = 0.01745329f; }
    public static class Time { public static float deltaTime; }
    public static class Screen { public static int width = 1, height = 1; }
    public static class Input {
        public static bool GetMouseButton(int b) { return false; }
        public static bool GetKey(KeyCode k) { return false; }
        public static bool GetKeyDown(KeyCode k) { return false; }
        public static float GetAxis(string a) { return 0; }
        public static Vector2 mouseScrollDelta = Vector2.zero;
        public static Vector2 mousePosition = Vector2.zero; }
    public static class Application { public static string streamingAssetsPath = ""; }
    public class Event { public EventType type; public Vector2 mousePosition; public void Use() { } public static Event current; }
    public class GUIContent { public GUIContent() { } public GUIContent(string t) { } }
    public class GUIStyle { public TextAnchor alignment; public bool wordWrap;
        public GUIStyle() { } public GUIStyle(GUIStyle other) { } }
    public class GUISkin {
        public GUIStyle box = new GUIStyle(); public GUIStyle label = new GUIStyle(); public GUIStyle button = new GUIStyle();
        public GUIStyle textField = new GUIStyle(); }
    public delegate void WindowFunction(int id);

    public static class GUI {
        public static GUISkin skin = new GUISkin();
        public static void Box(Rect r, string t) { } public static void Box(Rect r, GUIContent c) { }
        public static bool Button(Rect r, string t) { return false; }
        public static void Label(Rect r, string t) { }
        public static void DrawTexture(Rect r, Texture2D t, ScaleMode s, bool b, float a, Color c, float bw, float cr) { } }
    public class GUILayoutOption { }
    public static partial class GUILayout {
        public static void BeginHorizontal(GUIStyle s = null) { } public static void EndHorizontal() { }
        public static void BeginVertical() { } public static void EndVertical() { }
        public static bool Button(string t, params GUILayoutOption[] o) { return false; }
        public static void Label(string t, params GUILayoutOption[] o) { }
        public static void Label(string t, GUIStyle s, params GUILayoutOption[] o) { }
        public static bool Toggle(bool v, string t, params GUILayoutOption[] o) { return v; }
        public static string TextField(string t, params GUILayoutOption[] o) { return t; }
        public static void FlexibleSpace() { }
        public static Rect Window(int id, Rect r, WindowFunction f, string t) { return r; }
        public static void BeginArea(Rect r) { } public static void BeginArea(Rect r, string t, GUIStyle s) { } public static void BeginArea(Rect r, GUIStyle s) { } public static void EndArea() { }
        public static Vector2 BeginScrollView(Vector2 s) { return s; } public static void EndScrollView() { }
        public static GUILayoutOption Width(float w) { return null; } public static GUILayoutOption Height(float h) { return null; }
        public static GUILayoutOption ExpandWidth(bool b) { return null; } public static GUILayoutOption ExpandHeight(bool b) { return null; } }
    public static class GUILayoutUtility { public static Rect GetRect(float h, params GUILayoutOption[] o) { return new Rect(); }
        public static Rect GetRect(float w, float h, params GUILayoutOption[] o) { return new Rect(); } }
}

