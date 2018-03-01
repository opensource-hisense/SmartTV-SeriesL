LIST(APPEND WTF_SOURCES
    gibbon/MainThreadGibbon.cpp
    OSAllocatorPosix.cpp
    TCSystemAlloc.cpp
    ThreadIdentifierDataPthreads.cpp
    ThreadingPthreads.cpp
)

if(WTF_USE_ICU_UNICODE)
    LIST(APPEND WTF_SOURCES unicode/icu/CollatorICU.cpp)
    LIST(APPEND WTF_LIBRARIES icui18n icuuc)
else()
    LIST(APPEND WTF_SOURCES unicode/CollatorDefault.cpp)
endif()

LIST(APPEND WTF_LIBRARIES
    pthread
    ${CMAKE_DL_LIBS}
)


