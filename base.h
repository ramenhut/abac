
/*
//
// Copyright (c) 2002-2014 Joe Bertolami. All Right Reserved.
//
// base.h
//
//   Redistribution and use in source and binary forms, with or without
//   modification, are permitted provided that the following conditions are met:
//
//   * Redistributions of source code must retain the above copyright notice, this
//     list of conditions and the following disclaimer.
//
//   * Redistributions in binary form must reproduce the above copyright notice,
//     this list of conditions and the following disclaimer in the documentation
//     and/or other materials provided with the distribution.
//
//   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
//   AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
//   IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
//   DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
//   FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
//   DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
//   SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
//   CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
//   OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
//   OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
// Additional Information:
//
//   For more information, visit http://www.bertolami.com.
//
*/

#ifndef __EV_BASE_H__
#define __EV_BASE_H__

/**********************************************************************************
//
// Platform definitions
//
**********************************************************************************/

#if defined (_WIN32) || defined (_WIN64)
    #include "windows.h"                                    
   
    #pragma warning (disable : 4244)                      // conversion, possible loss of data   
    #pragma warning (disable : 4018)                      // signed / unsigned mismatch
    #pragma warning (disable : 4996)                      // deprecated interfaces
    #pragma warning (disable : 4221)                      // empty translation unit
    #pragma warning (disable : 4273)                      // inconsistent linkage

    #define EVX_PLATFORM_WINDOWS                          // building a Windows application
#elif defined (__APPLE__)
    #include "TargetConditionals.h"
    #import "Foundation/Foundation.h"
    #include "unistd.h"
    #include "sys/types.h"
    #include "ctype.h"

    #if TARGET_OS_IPHONE || TARGET_IPHONE_SIMULATOR
        #define EVX_PLATFORM_IOS                              // building an application for iOS
    #elif TARGET_OS_MAC
        #define EVX_PLATFORM_MACOSX                           // building a Mac OSX application
    #endif
#else
    #error "Unsupported target platform detected."
#endif

/**********************************************************************************
//
// Debug definitions
//
**********************************************************************************/

#if defined (EVX_PLATFORM_WINDOWS)
    #ifdef _DEBUG
        #define EVX_DEBUG _DEBUG
    #elif defined (DEBUG)
        #define EVX_DEBUG DEBUG
    #endif
    #if defined(EVX_DEBUG) && !defined(debug_break)
        #define debug_break __debugbreak
    #endif
    #define __EVX_FUNCTION__  __FUNCTION__
#elif defined (EVX_PLATFORM_IOS) || defined (EVX_PLATFORM_MACOSX)
   #ifdef DEBUG
       #define EVX_DEBUG DEBUG
       #if !defined(debug_break)
           #define debug_break() __builtin_trap()
       #endif
    #endif
    #define __EVX_FUNCTION__ __func__
#endif

/**********************************************************************************
//
// Standard headers
//
**********************************************************************************/

#include "stdio.h"
#include "stdlib.h"
#include "stdarg.h"

/**********************************************************************************
//
// Standard types
//
**********************************************************************************/

namespace evx {

#if defined (EVX_PLATFORM_WINDOWS)
    typedef INT64 int64;		  
    typedef INT32 int32;		    
    typedef INT16 int16;		     
    typedef INT8  int8; 

    typedef UINT64 uint64;		      
    typedef UINT32 uint32;		        
    typedef UINT16 uint16;		        
    typedef UINT8  uint8;
#elif defined (EVX_PLATFORM_IOS) || defined (EVX_PLATFORM_MACOSX)
    typedef int64_t int64;		
    typedef int32_t int32;		
    typedef int16_t int16;		
    typedef int8_t  int8; 

    typedef u_int64_t uint64;	    
    typedef u_int32_t uint32;	    
    typedef u_int16_t uint16;	    
    typedef u_int8_t uint8;	 
#endif

typedef float float32;         
typedef double float64;           
typedef wchar_t wchar;

} // namespace evx

/**********************************************************************************
//
// Status codes
//
**********************************************************************************/

namespace evx { typedef uint8 evx_status; }

#define EVX_SUCCESS                                 (0)
#define EVX_ERROR_INVALIDARG                        (1)
#define EVX_ERROR_NOTIMPL                           (2)
#define EVX_ERROR_OUTOFMEMORY                       (3)
#define EVX_ERROR_UNDEFINED                         (4)
#define EVX_ERROR_HARDWAREFAIL                      (5)
#define EVX_ERROR_INVALID_INDEX                     (6)
#define EVX_ERROR_CAPACITY_LIMIT                    (7)
#define EVX_ERROR_INVALID_RESOURCE                  (8)
#define EVX_ERROR_OPERATION_TIMEDOUT                (9)
#define EVX_ERROR_EXECUTION_FAILURE                 (10)
#define EVX_ERROR_PERMISSION_DENIED                 (11)
#define EVX_ERROR_IO_FAILURE                        (12)
#define EVX_ERROR_RESOURCE_UNREACHABLE              (13)
#define EVX_ERROR_SYSTEM_FAILURE                    (14)
#define EVX_ERROR_NOT_READY                         (15)
#define EVX_ERROR_OPERATION_COMPLETED               (16)
#define EVX_ERROR_RESOURCE_UNUSED                   (17)

/**********************************************************************************
//
// Debug support
//
**********************************************************************************/

#ifdef EVX_DEBUG
    #define EVX_PARAM_CHECK (1)
    #define evx_err(fmt, ...) do { printf("[EVX-ERR] "); \
                                   printf(fmt, ##__VA_ARGS__); \
                                   printf("\n"); debug_break(); \
                              } while(0)

    #define evx_msg(fmt, ...) do { printf("[EVX-MSG] "); \
                                   printf(fmt, ##__VA_ARGS__); \
                                   printf("\n"); \
                              } while(0)
#else
    #define EVX_PARAM_CHECK (0)
    #define evx_err(fmt, ...)                              
    #define evx_msg(fmt, ...)  
#endif 

#define EVX_ERROR_CREATE_STRING(x) ((char *) #x)
#define evx_post_error(x) post_error_i(x, EVX_ERROR_CREATE_STRING(x), __EVX_FUNCTION__, (char *) __FILE__, __LINE__)

namespace evx {

inline uint32 post_error_i(uint8 error, const char *error_string, const char *function, const char *filename, uint32 line) 
{
#ifdef EVX_DEBUG      
    const char *path = filename;
    for (int32 i = (int32) strlen(filename); i >= 0; --i) 
    {
        if (filename[ i ] == '/') 
            break;

        path = &filename[i];    
    }

    evx_err("*** RIP *** %s @ %s in %s:%i", error_string, function, path, line);
#endif
    return error;
}

} // namespace evx

/**********************************************************************************
//
// Standard helpers
//
**********************************************************************************/

#define EVX_DISABLE_COPY_AND_ASSIGN(type) \
    type(const type &rvalue); \
    type &operator = (const type &rvalue);

#endif // __EV_BASE_H__