/*
  OrbitCamera.cs —— 自由视角控制 (新建, 2026)
  操作: 鼠标右键拖拽(或 Alt+左键) 旋转
        鼠标中键拖拽(或 右键+Shift) 平移
        滚轮 缩放
        F 键 将当前模型置于视野中心
*/
using UnityEngine;

namespace ModelViewer
{
    public class OrbitCamera : MonoBehaviour
    {
        public Vector3 focusPoint = Vector3.zero; // 注视点
        public float distance = 8f;               // 相机与注视点距离
        public float yaw = 45f;
        public float pitch = 30f;
        public float minDistance = 0.01f;
        public float maxDistance = 100000f;
        public float rotateSpeed = 2.5f;
        public float panSpeed = 1.0f;
        public float zoomSpeed = 1.2f;

        void Start()
        {
            transform.position = focusPoint + Quaternion.Euler(pitch, yaw, 0) * new Vector3(0, 0, -distance);
            transform.LookAt(focusPoint);
        }

        void LateUpdate()
        {
            // ---- 旋转: 右键 或 Alt+左键 ----
            bool rot = Input.GetMouseButton(1) || (Input.GetKey(KeyCode.LeftAlt) && Input.GetMouseButton(0));
            if (rot)
            {
                float dx = Input.GetAxis("Mouse X"), dy = Input.GetAxis("Mouse Y");
                yaw += dx * rotateSpeed;
                pitch -= dy * rotateSpeed;
                pitch = Mathf.Clamp(pitch, -89f, 89f);
            }

            // ---- 平移: 中键 或 右键+Shift ----
            bool pan = Input.GetMouseButton(2) || (Input.GetKey(KeyCode.LeftShift) && Input.GetMouseButton(1));
            if (pan)
            {
                float dx = -Input.GetAxis("Mouse X"), dy = -Input.GetAxis("Mouse Y");
                // 平移量与距离成正比(远近一致的拖拽手感)
                float k = distance * 0.0015f * panSpeed * Mathf.Max(Screen.height, 1);
                Vector3 right = transform.right, up = transform.up;
                focusPoint += right * dx * k + up * dy * k;
            }

            // ---- 缩放: 滚轮 ----
            float wheel = Input.mouseScrollDelta.y * 0.1f;
            if (Mathf.Abs(wheel) > 0.001f)
                distance = Mathf.Clamp(distance * Mathf.Pow(zoomSpeed, -wheel), minDistance, maxDistance);

            Quaternion rotQ = Quaternion.Euler(pitch, yaw, 0);
            transform.position = focusPoint + rotQ * new Vector3(0, 0, -distance);
            transform.rotation = rotQ;
        }

        /// <summary>将指定物体置于视野中心 (F 键功能)</summary>
        public void FrameGameObject(GameObject go)
        {
            if (go == null) return;
            Renderer r = go.GetComponentInChildren<Renderer>();
            if (r != null) FrameBounds(r.bounds);
        }

        /// <summary>将模型包围盒置于视野中心</summary>
        public void FrameBounds(Bounds b)
        {
            focusPoint = b.center;
            float fov = GetComponent<Camera>() != null ? GetComponent<Camera>().fieldOfView : 60f;
            float fit = b.extents.magnitude / Mathf.Max(0.01f, Mathf.Tan(fov * 0.5f * Mathf.Deg2Rad));
            distance = Mathf.Clamp(fit * 1.4f, minDistance, maxDistance);
        }
    }
}
