
ADD_DEFINITIONS(-DWTF_USE_V8=0)

LIST(APPEND JavaScriptCore_SOURCES
    jit/ExecutableAllocatorFixedVMPool.cpp
    jit/ExecutableAllocator.cpp
)

LIST(APPEND JavaScriptCore_LIBRARIES
    ${ICU_I18N_LIBRARIES}
)

LIST(APPEND JavaScriptCore_LINK_FLAGS
    ${ECORE_LDFLAGS}
)
