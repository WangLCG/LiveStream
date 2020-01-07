/*
 * Car eye 车辆管理平台: www.car-eye.cn
 * Car eye 开源网址: https://github.com/Car-eye-team
 * CEDebug.h
 *
 * Author: Wgj
 * Date: 2019-03-04 22:21
 * Copyright 2019
 *
 * 调试信息分级输出, 仿照Android输出方式
 */
 
#ifndef __CE_DEBUG_H_
#define __CE_DEBUG_H_

#include <stdio.h>

 // Verbose级别, 详细信息, 所有级别的信息都将输出
#define D_VERBOSE       0
// Debug级别, 调试以及以上等级信息输出
#define D_DEBUG         5
// Info级别, 通知信息及以上等级信息输出
#define D_INFO          10
// Warn级别, 警告信息及以上等级信息输出
#define D_WARN          15
// Error级别, 错误信息以及以上等级信息输出
#define D_ERROR         20
// DISABLE级别, 禁止所有调试信息从调试口输出
#define D_DISABLE       99

// DEBUG选项时输出调试信息选项
#ifdef _DEBUG
#define DEBUG_LEVEL D_VERBOSE
#else
// 当前的调试信息输出级别, 只输出该级别及以上级别的信息
#define DEBUG_LEVEL D_VERBOSE
#endif

// 输出VERBOSE详细级别信息
#if DEBUG_LEVEL > D_VERBOSE
#define DEBUG_V(aFrmt, ...)
#define _DEBUG_V(aFrmt, ...)
#else
#define DEBUG_V(aFrmt, ...)     fprintf(stdout, "V:[%s:%d] "aFrmt"\r\n", __FILE__, __LINE__, ##__VA_ARGS__)
#define _DEBUG_V(aFrmt, ...)    fprintf(stdout, aFrmt, ##__VA_ARGS__)
#endif

// 输出DEBUG调试级别信息
#if DEBUG_LEVEL > D_DEBUG
#define DEBUG_D(aFrmt, ...)
#define _DEBUG_D(aFrmt, ...)
#else
#define DEBUG_D(aFrmt, ...)     fprintf(stdout, "D:[%s:%d] "aFrmt"\r\n", __FILE__, __LINE__, ##__VA_ARGS__)
#define _DEBUG_D(aFrmt, ...)    fprintf(stdout, aFrmt, ##__VA_ARGS__)
#endif

// 输出INFO通知级别信息
#if DEBUG_LEVEL > D_INFO
#define DEBUG_I(aFrmt, ...)
#define _DEBUG_I(aFrmt, ...)
#else
#define DEBUG_I(aFrmt, ...)     fprintf(stdout, "I:[%s:%d] "aFrmt"\r\n", __FILE__, __LINE__, ##__VA_ARGS__)
#define _DEBUG_I(aFrmt, ...)    fprintf(stdout, aFrmt, ##__VA_ARGS__)
#endif

// 输出WARN警告级别信息
#if DEBUG_LEVEL > D_WARN
#define DEBUG_W(aFrmt, ...)
#define _DEBUG_W(aFrmt, ...)
#else
#define DEBUG_W(aFrmt, ...)     fprintf(stderr, "W:[%s:%d] "aFrmt"\r\n", __FILE__, __LINE__, ##__VA_ARGS__)
#define _DEBUG_W(aFrmt, ...)    fprintf(stderr, aFrmt, ##__VA_ARGS__)
#endif

// 输出ERROR错误级别信息
#if DEBUG_LEVEL > D_ERROR
#define DEBUG_E(aFrmt, ...)
#define _DEBUG_E(aFrmt, ...)
#else
#define DEBUG_E(aFrmt, ...)     fprintf(stderr, "E:[%s:%d] "aFrmt"\r\n", __FILE__, __LINE__, ##__VA_ARGS__)
#define _DEBUG_E(aFrmt, ...)    fprintf(stderr, aFrmt, ##__VA_ARGS__)
#endif

#endif