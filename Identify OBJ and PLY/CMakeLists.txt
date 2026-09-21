# CMakeLists.txt —— 跨平台编译 Unity 原生插件 ModelReader (新建文件, 2026)
# Windows:  cmake -B build -A x64 && cmake --build build --config Release
#           -> 产物 build/Release/ModelReader.dll
# macOS  :  cmake -B build && cmake --build build
#           -> 产物 build/ModelReader.bundle (放入 UnityAssets/Plugins/macOS/)
cmake_minimum_required(VERSION 3.12)
project(ModelReader CXX)

set(CMAKE_POSITION_INDEPENDENT_CODE ON)

add_library(ModelReader MODULE
    OBJ.cpp
    glm.cpp
    hply.cpp
    ReadPly.cpp
    ModelBridge.cpp
)

target_compile_definitions(ModelReader PRIVATE
    NO_OPENGL_RENDER
    MODEL_BRIDGE_EXPORTS
    _CRT_SECURE_NO_WARNINGS
    _CRT_NONSTDC_NO_DEPRECATE
)

if (APPLE)
    # macOS 输出 ModelReader.bundle (Unity 插件包格式)
    set_target_properties(ModelReader PROPERTIES
        BUNDLE TRUE
        PREFIX ""
        OUTPUT_NAME "ModelReader"
    )
elseif (WIN32)
    set_target_properties(ModelReader PROPERTIES PREFIX "")
endif()

# 可选: 编译控制台测试程序  cmake --build build --target test_bridge
add_executable(test_bridge test_bridge.cpp)
target_compile_definitions(test_bridge PRIVATE NO_OPENGL_RENDER)
target_link_libraries(test_bridge PRIVATE ModelReader)
