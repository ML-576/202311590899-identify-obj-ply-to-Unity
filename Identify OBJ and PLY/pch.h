/*
  pch.h —— 预编译头（新建文件）
  原工程缺少此文件，导致 glm.cpp / OBJ.cpp / hply.cpp / ReadPly.cpp 中
  的 #include "pch.h" 报错。此文件同时提供：
    1. 关闭 MSVC 安全警告 (fopen/strcpy 等旧式 C 函数)
    2. 无 MFC 环境下 DEBUG_NEW 的替代定义 (OBJ.cpp 使用)
    3. 常用 C 标准库头文件
*/
#ifndef PCH_H_
#define PCH_H_

#define _CRT_SECURE_NO_WARNINGS
#define _CRT_NONSTDC_NO_DEPRECATE
#define _SCL_SECURE_NO_WARNINGS

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <math.h>
#include <assert.h>
#include <ctype.h>

/* OBJ.cpp 在 Debug 配置下使用 MFC 的 DEBUG_NEW，无 MFC 时用原生 new 替代 */
#ifndef DEBUG_NEW
#define DEBUG_NEW new
#endif

/* 2026新增: macOS/Linux 下没有 _strdup, 映射到标准 strdup */
#ifndef _MSC_VER
#define _strdup strdup
#endif

#endif /* PCH_H_ */
