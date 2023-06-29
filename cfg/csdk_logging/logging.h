/*
 * Copyright of Amazon Web Services, Inc. (AWS) 2022
 *
 * This code is licensed under the AWS Intellectual Property License, which can
 * be found here: https://aws.amazon.com/legal/aws-ip-license-terms/; provided
 * that AWS grants you a limited, royalty-free, revocable, non-exclusive,
 * non-sublicensable, non-transferrable license to modify the code for internal
 * testing purposes. Your receipt of this code is subject to any non-disclosure
 * (or similar) agreement between you and AWS.
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
