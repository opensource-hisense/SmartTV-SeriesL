CMAKE_MINIMUM_REQUIRED(VERSION 2.8)
PROJECT(JavaScriptCore)

if(NOT GIBBON_SCRIPT_JSC_WARNINGS)
    add_definitions(-w)
endif()

# Disable pedantic warnings if enabled
foreach(var CMAKE_C_FLAGS;CMAKE_CXX_FLAGS)
    string(REPLACE "-Wpedantic" "" ${var} "${${var}}")
    string(REPLACE "-pedantic" "" ${var} "${${var}}")
endforeach()

SET(JAVASCRIPTCORE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/Source/JavaScriptCore")

if(JAVASCRIPTCORE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE ${JAVASCRIPTCORE_BUILD_TYPE})
endif()

string(TOUPPER "CMAKE_LINKER_FLAGS_${CMAKE_BUILD_TYPE}" LFLAGS_VAR)
string(TOUPPER "CMAKE_C_FLAGS_${CMAKE_BUILD_TYPE}" CFLAGS_VAR)
string(TOUPPER "CMAKE_CXX_FLAGS_${CMAKE_BUILD_TYPE}" CXXFLAGS_VAR)

if(GIBBON_SCRIPT_JSC_DEBUG_SYMBOLS)
    set(${CXXFLAGS_VAR} "${${CXXFLAGS_VAR}} -g")
    set(${CFLAGS_VAR} "${${CFLAGS_VAR}} -g")
endif()

if(GIBBON_SCRIPT_JSC_ASSERTIONS)
  foreach(var CMAKE_C_FLAGS_RELEASE;CMAKE_CXX_FLAGS_RELEASE)
      string(REPLACE "-DNDEBUG" "" ${var} "${${var}}")
  endforeach()
endif()

if(BUILD_GPROF)
    set(${LFLAGS_VAR} "${${LFLAGS_VAR}} -pg")
    set(${CFLAGS_VAR} "${${CFLAGS_VAR}} -g -pg")
    set(${CXXFLAGS_VAR} "${${CXXFLAGS_VAR}} -g -pg")
endif()

if(BUILD_GPERFTOOLS)
    set(${CXXFLAGS_VAR} "${${CXXFLAGS_VAR}} -g")
    set(${CFLAGS_VAR} "${${CFLAGS_VAR}} -g")
endif()

SET(WTF_DIR "${CMAKE_CURRENT_SOURCE_DIR}/Source/WTF")
SET(JAVASCRIPTCORE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/Source/JavaScriptCore")
SET(DERIVED_SOURCES_DIR "${CMAKE_CURRENT_BINARY_DIR}/DerivedSources")
SET(DERIVED_SOURCES_JAVASCRIPTCORE_DIR "${CMAKE_CURRENT_BINARY_DIR}/DerivedSources/JavaScriptCore")

SET(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/Source/cmake")

INCLUDE(WebKitMacros)
INCLUDE(WebKitFS)
INCLUDE(WebKitHelpers)
INCLUDE(WebKitFeatures)

SET(PORT "Gibbon" CACHE STRING "choose which WebKit port to build (one of ${ALL_PORTS})")

if(GIBBON_TCMALLOC)
    add_definitions(-DWTF_USE_GIBBON_TCMALLOC)
endif()

INCLUDE_DIRECTORIES("${CMAKE_CURRENT_SOURCE_DIR}/Source")
INCLUDE_DIRECTORIES("${CMAKE_CURRENT_BINARY_DIR}/Source")

STRING(TOLOWER ${PORT} WEBKIT_PORT_DIR)


# -----------------------------------------------------------------------------
# Find common packages (used by all ports)
# -----------------------------------------------------------------------------
#FIND_PACKAGE(BISON REQUIRED)
#FIND_PACKAGE(FLEX REQUIRED)
#FIND_PACKAGE(Perl REQUIRED)
FIND_PACKAGE(PythonInterp REQUIRED)

# -----------------------------------------------------------------------------
# Determine the target processor
# -----------------------------------------------------------------------------
IF ("${JAVASCRIPTCORE_ARCH}" STREQUAL "")
    SET(JAVASCRIPTCORE_ARCH "${CMAKE_SYSTEM_PROCESSOR}")
ENDIF()
STRING(TOLOWER ${JAVASCRIPTCORE_ARCH} LOWERCASE_CMAKE_SYSTEM_PROCESSOR)
IF (LOWERCASE_CMAKE_SYSTEM_PROCESSOR MATCHES "^arm")
    SET(WTF_CPU_ARM 1)
ELSEIF (LOWERCASE_CMAKE_SYSTEM_PROCESSOR MATCHES "^mips")
    SET(WTF_CPU_MIPS 1)
ELSEIF (LOWERCASE_CMAKE_SYSTEM_PROCESSOR MATCHES "(x86_64|amd64)")
    SET(WTF_CPU_X86_64 1)
ELSEIF (LOWERCASE_CMAKE_SYSTEM_PROCESSOR MATCHES "(i[3-6]86|x86)")
    SET(WTF_CPU_X86 1)
ELSE ()
    MESSAGE(FATAL_ERROR "Unknown CPU '${CMAKE_SYSTEM_PROCESSOR}'")
ENDIF ()

# -----------------------------------------------------------------------------
# Determine the operating system
# -----------------------------------------------------------------------------
IF (UNIX)
    IF (APPLE)
        SET(WTF_OS_MAC_OS_X 1)
    ELSE ()
        SET(WTF_OS_UNIX 1)
    ENDIF ()
ELSEIF (CMAKE_SYSTEM_NAME MATCHES "Windows")
    SET(WTF_OS_WINDOWS 1)
ELSEIF (CMAKE_SYSTEM_NAME MATCHES "QNX")
    SET(WTF_OS_QNX 1)
    SET(WTF_OS_UNIX 1)
ELSE ()
    MESSAGE(FATAL_ERROR "Unknown OS '${CMAKE_SYSTEM_NAME}'")
ENDIF ()


# -----------------------------------------------------------------------------
# Default target names (can be overrriden in Options${PORT}.cmake file)
# -----------------------------------------------------------------------------
SET(JSC_EXECUTABLE_NAME JSC)
SET(WTF_LIBRARY_NAME WTF)
SET(JavaScriptCore_LIBRARY_NAME JavaScriptCore)
SET(WTF_LIBRARY_TYPE STATIC)
SET(JavaScriptCore_LIBRARY_TYPE STATIC)

# -----------------------------------------------------------------------------
# Port-specific options
# -----------------------------------------------------------------------------
INCLUDE(OptionsCommon)
INCLUDE(Options${PORT})

ADD_SUBDIRECTORY(Source/JavaScriptCore)
ADD_SUBDIRECTORY(Source/WTF)

# -----------------------------------------------------------------------------
# Create the configuration file
# -----------------------------------------------------------------------------
configure_file_if_different(Source/cmakeconfig.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/Source/cmakeconfig.h)
# If building outside of nrdp use
# CONFIGURE_FILE(Source/cmakeconfig.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/Source/cmakeconfig.h @ONLY)
