# (c) 1997-2015 Netflix, Inc.  All content herein is protected by
# U.S. copyright and other applicable intellectual property laws and
# may not be copied without the express permission of Netflix, Inc.,
# which reserves all rights.  Reuse of any of this content for any
# purpose without the permission of Netflix, Inc. is strictly
# prohibited.

macro(REMOVE_CFLAG flag)
    string(TOUPPER "CMAKE_C_FLAGS_${CMAKE_BUILD_TYPE}" CFLAGS_VAR)
    string(TOUPPER "CMAKE_CXX_FLAGS_${CMAKE_BUILD_TYPE}" CXXFLAGS_VAR)

    string (REPLACE "${flag}" "" ${CFLAGS_VAR} ${${CFLAGS_VAR}})
    string (REPLACE "${flag}" "" ${CXXFLAGS_VAR} ${${CXXFLAGS_VAR}})
endmacro()

#-------------------------------------------------------------------------------

add_definitions(-DBUILDING_GIBBON__)

remove_cflag("-pedantic")

set(WTF_USE_PTHREADS 1)
add_definitions(-DWTF_USE_PTHREADS=1)

set(WTF_USE_ICU_UNICODE 1)
add_definitions(-DWTF_USE_ICU_UNICODE=1)

if(NOT JAVASCRIPTCORE_JIT)
    set(JAVASCRIPTCORE_JIT OFF)
endif()

if(GIBBON_SCRIPT_JSC_DYNAMIC)
    set(WTF_LIBRARY_TYPE STATIC)
    set(JavaScriptCore_LIBRARY_TYPE SHARED)
else()
    set(WTF_LIBRARY_TYPE STATIC)
    set(JavaScriptCore_LIBRARY_TYPE STATIC)
endif()

if(HAVE_MALLOC_TRIM)
    add_definitions(-DHAVE_MALLOC_TRIM)
endif()
if(HAVE_MALLOC_USABLE_SIZE)
    add_definitions(-DHAVE_MALLOC_USABLE_SIZE)
endif()

WEBKIT_OPTION_BEGIN()
WEBKIT_OPTION_DEFAULT_PORT_VALUE(ENABLE_JAVASCRIPT_DEBUGGER OFF)
WEBKIT_OPTION_DEFAULT_PORT_VALUE(ENABLE_FAST_MALLOC ON)
WEBKIT_OPTION_DEFAULT_PORT_VALUE(ENABLE_JIT ${JAVASCRIPTCORE_JIT})
WEBKIT_OPTION_DEFAULT_PORT_VALUE(ENABLE_YARR ${JAVASCRIPTCORE_JIT})
WEBKIT_OPTION_DEFAULT_PORT_VALUE(ENABLE_YARR_JIT ${JAVASCRIPTCORE_JIT})
WEBKIT_OPTION_END()
