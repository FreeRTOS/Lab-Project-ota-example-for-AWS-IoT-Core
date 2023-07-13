/*
 * Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
 * SPDX-License-Identifier: MIT-0
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of this
 * software and associated documentation files (the "Software"), to deal in the Software
 * without restriction, including without limitation the rights to use, copy, modify,
 * merge, publish, distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
 * PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
 * OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include <stdio.h>

#define LOG_NONE  0U
#define LOG_ERROR 1U
#define LOG_WARN  2U
#define LOG_INFO  3U
#define LOG_DEBUG 4U

#ifndef LIBRARY_LOG_NAME
    #error "Please define LIBRARY_LOG_NAME for the compilation unit."
#endif

#ifndef LIBRARY_LOG_LEVEL
    #error "Please define LIBRARY_LOG_LEVEL for the compilation unit."
#endif

#define LOG_UNPACK( ... ) __VA_ARGS__
#define LOG_GENERIC_INNER( prefix, format, ... )                  \
    fprintf( stderr,                                              \
             prefix " " LIBRARY_LOG_NAME ": " format "\033[0m\n", \
             ##__VA_ARGS__ )
#define LOG_GENERIC( ... ) LOG_GENERIC_INNER( __VA_ARGS__ )

#if LIBRARY_LOG_LEVEL >= LOG_DEBUG
    #define LogDebug( body ) LOG_GENERIC( "\033[0;34mD", LOG_UNPACK body )
#else
    #define LogDebug( body )
#endif

#if LIBRARY_LOG_LEVEL >= LOG_INFO
    #define LogInfo( body ) LOG_GENERIC( "\033[0;32mI", LOG_UNPACK body )
#else
    #define LogInfo( body )
#endif

#if LIBRARY_LOG_LEVEL >= LOG_WARN
    #define LogWarn( body ) LOG_GENERIC( "\033[1;33mW", LOG_UNPACK body )
#else
    #define LogWarn( body )
#endif

#if LIBRARY_LOG_LEVEL >= LOG_ERROR
    #define LogError( body ) LOG_GENERIC( "\033[1;31mE", LOG_UNPACK body )
#else
    #define LogError( body )
#endif
