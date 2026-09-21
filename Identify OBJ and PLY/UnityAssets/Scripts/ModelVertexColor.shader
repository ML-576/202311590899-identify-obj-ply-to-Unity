// ModelVertexColor.shader —— 带顶点颜色的漫反射着色器 (新建, 2026)
// 用于显示带顶点颜色的 PLY 模型 (内置渲染管线)
// 不支持顶点颜色时自动回退 Standard/Diffuse
Shader "Custom/ModelVertexColor"
{
    Properties
    {
        _MainTex ("Texture", 2D) = "white" {}
    }
    SubShader
    {
        Tags { "RenderType"="Opaque" }
        LOD 200

        CGPROGRAM
        #pragma surface surf Lambert
        #pragma target 2.0

        struct Input
        {
            float2 uv_MainTex;
            float4 color : COLOR;   // 网格顶点颜色
        };

        sampler2D _MainTex;

        void surf (Input IN, inout SurfaceOutput o)
        {
            fixed4 tex = tex2D(_MainTex, IN.uv_MainTex);
            o.Albedo = tex.rgb * IN.color.rgb;
            o.Alpha = 1.0;
        }
        ENDCG
    }
    Fallback "Diffuse"
}
