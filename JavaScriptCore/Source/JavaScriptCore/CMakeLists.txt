# Do not throw warnings when using CMake > 2.8.12
if(POLICY CMP0022)
    cmake_policy(SET CMP0022 OLD)
endif()

SET(JavaScriptCore_INCLUDE_DIRECTORIES
    "${CMAKE_BINARY_DIR}"
    "${JAVASCRIPTCORE_DIR}"
    "${JAVASCRIPTCORE_DIR}/API"
    "${JAVASCRIPTCORE_DIR}/ForwardingHeaders"
    "${JAVASCRIPTCORE_DIR}/assembler"
    "${JAVASCRIPTCORE_DIR}/bytecode"
    "${JAVASCRIPTCORE_DIR}/bytecompiler"
    "${JAVASCRIPTCORE_DIR}/dfg"
    "${JAVASCRIPTCORE_DIR}/heap"
    "${JAVASCRIPTCORE_DIR}/debugger"
    "${JAVASCRIPTCORE_DIR}/interpreter"
    "${JAVASCRIPTCORE_DIR}/jit"
    "${JAVASCRIPTCORE_DIR}/llint"
    "${JAVASCRIPTCORE_DIR}/parser"
    "${JAVASCRIPTCORE_DIR}/profiler"
    "${JAVASCRIPTCORE_DIR}/runtime"
    "${JAVASCRIPTCORE_DIR}/tools"
    "${JAVASCRIPTCORE_DIR}/yarr"
    "${WTF_DIR}"
    "${DERIVED_SOURCES_JAVASCRIPTCORE_DIR}"
    "${CMAKE_SOURCE_DIR}/Source"
)

SET(JavaScriptCore_SOURCES
    API/JSBase.cpp
    API/JSCallbackConstructor.cpp
    API/JSCallbackFunction.cpp
    API/JSCallbackObject.cpp
    API/JSClassRef.cpp
    API/JSContextRef.cpp
    API/JSObjectRef.cpp
    API/JSProfilerPrivate.cpp
    API/JSStringRef.cpp
    API/JSValueRef.cpp
    API/JSWeakObjectMapRefPrivate.cpp
    API/OpaqueJSString.cpp

    API/APICast.h
    API/APIShims.h
    API/JavaScript.h
    API/JavaScriptCore.h
    API/JSBase.h
    API/JSBasePrivate.h
    API/JSCallbackConstructor.h
    API/JSCallbackFunction.h
    API/JSCallbackObject.h
    API/JSCallbackObjectFunctions.h
    API/JSClassRef.h
    API/JSContextRef.h
    API/JSContextRefPrivate.h
    API/JSObjectRef.h
    API/JSObjectRefPrivate.h
    API/JSProfilerPrivate.h
    API/JSRetainPtr.h
    API/JSStringRef.h
    API/JSValueRef.h
    API/JSWeakObjectMapRefInternal.h
    API/JSWeakObjectMapRefPrivate.h
    API/OpaqueJSString.h
    API/WebKitAvailability.h

    bytecode/CallLinkInfo.cpp
    bytecode/CallLinkStatus.cpp
    bytecode/CodeBlock.cpp
    bytecode/DFGExitProfile.cpp
    bytecode/ExecutionCounter.cpp
    bytecode/GetByIdStatus.cpp
    bytecode/JumpTable.cpp
    bytecode/LazyOperandValueProfile.cpp
    bytecode/MethodCallLinkInfo.cpp
    bytecode/MethodCallLinkStatus.cpp
    bytecode/MethodOfGettingAValueProfile.cpp
    bytecode/Opcode.cpp
    bytecode/PolymorphicPutByIdList.cpp
    bytecode/PredictedType.cpp
    bytecode/PutByIdStatus.cpp
    bytecode/SamplingTool.cpp
    bytecode/StructureStubInfo.cpp

    bytecode/BytecodeConventions.h
    bytecode/CallLinkInfo.h
    bytecode/CallLinkStatus.h
    bytecode/CallReturnOffsetToBytecodeOffset.h
    bytecode/CodeBlock.h
    bytecode/CodeOrigin.h
    bytecode/CodeType.h
    bytecode/DataFormat.h
    bytecode/DFGExitProfile.h
    bytecode/EvalCodeCache.h
    bytecode/ExecutionCounter.h
    bytecode/ExpressionRangeInfo.h
    bytecode/GetByIdStatus.h
    bytecode/GlobalResolveInfo.h
    bytecode/HandlerInfo.h
    bytecode/Instruction.h
    bytecode/JumpTable.h
    bytecode/LazyOperandValueProfile.h
    bytecode/LineInfo.h
    bytecode/LLIntCallLinkInfo.h
    bytecode/MethodCallLinkInfo.h
    bytecode/MethodCallLinkStatus.h
    bytecode/MethodOfGettingAValueProfile.h
    bytecode/Opcode.h
    bytecode/Operands.h
    bytecode/PolymorphicPutByIdList.h
    bytecode/PredictedType.h
    bytecode/PutByIdStatus.h
    bytecode/PutKind.h
    bytecode/SamplingTool.h
    bytecode/StructureSet.h
    bytecode/StructureStubInfo.h
    bytecode/ValueProfile.h
    bytecode/ValueRecovery.h
    bytecode/VirtualRegister.h

    bytecompiler/BytecodeGenerator.cpp
    bytecompiler/NodesCodegen.cpp

    bytecompiler/BytecodeGenerator.h
    bytecompiler/Label.h
    bytecompiler/LabelScope.h
    bytecompiler/RegisterID.h

    dfg/DFGAbstractState.cpp
    dfg/DFGAssemblyHelpers.cpp
    dfg/DFGByteCodeParser.cpp
    dfg/DFGCapabilities.cpp
    dfg/DFGCFAPhase.cpp
    dfg/DFGCorrectableJumpPoint.cpp
    dfg/DFGCSEPhase.cpp
    dfg/DFGDriver.cpp
    dfg/DFGFixupPhase.cpp
    dfg/DFGGraph.cpp
    dfg/DFGJITCompiler.cpp
    dfg/DFGNodeFlags.cpp
    dfg/DFGOSREntry.cpp
    dfg/DFGOSRExit.cpp
    dfg/DFGOSRExitCompiler.cpp
    dfg/DFGOSRExitCompiler32_64.cpp
    dfg/DFGOSRExitCompiler64.cpp
    dfg/DFGOperations.cpp
    dfg/DFGPhase.cpp
    dfg/DFGPredictionPropagationPhase.cpp
    dfg/DFGRedundantPhiEliminationPhase.cpp
    dfg/DFGRepatch.cpp
    dfg/DFGSpeculativeJIT.cpp
    dfg/DFGSpeculativeJIT32_64.cpp
    dfg/DFGSpeculativeJIT64.cpp
    dfg/DFGThunks.cpp
    dfg/DFGVirtualRegisterAllocationPhase.cpp

    dfg/DFGAbstractState.h
    dfg/DFGAbstractValue.h
    dfg/DFGAdjacencyList.h
    dfg/DFGArgumentPosition.h
    dfg/DFGAssemblyHelpers.h
    dfg/DFGBasicBlock.h
    dfg/DFGByteCodeCache.h
    dfg/DFGByteCodeParser.h
    dfg/DFGCapabilities.h
    dfg/DFGCCallHelpers.h
    dfg/DFGCFAPhase.h
    dfg/DFGCommon.h
    dfg/DFGCorrectableJumpPoint.h
    dfg/DFGCSEPhase.h
    dfg/DFGDoubleFormatState.h
    dfg/DFGDriver.h
    dfg/DFGEdge.h
    dfg/DFGFixupPhase.h
    dfg/DFGFPRInfo.h
    dfg/DFGGenerationInfo.h
    dfg/DFGGPRInfo.h
    dfg/DFGGraph.h
    dfg/DFGInsertionSet.h
    dfg/DFGJITCompiler.h
    dfg/DFGNode.h
    dfg/DFGNodeFlags.h
    dfg/DFGNodeType.h
    dfg/DFGOperations.h
    dfg/DFGOSREntry.h
    dfg/DFGOSRExit.h
    dfg/DFGOSRExitCompiler.h
    dfg/DFGPhase.h
    dfg/DFGPredictionPropagationPhase.h
    dfg/DFGRedundantPhiEliminationPhase.h
    dfg/DFGRegisterBank.h
    dfg/DFGRepatch.h
    dfg/DFGScoreBoard.h
    dfg/DFGSpeculativeJIT.h
    dfg/DFGThunks.h
    dfg/DFGVariableAccessData.h
    dfg/DFGVirtualRegisterAllocationPhase.h

    heap/BlockAllocator.cpp
    heap/CopiedSpace.cpp
    heap/ConservativeRoots.cpp
    heap/DFGCodeBlocks.cpp
    heap/HandleSet.cpp
    heap/HandleStack.cpp
    heap/Heap.cpp
    heap/MachineStackMarker.cpp
    heap/MarkedAllocator.cpp
    heap/MarkedBlock.cpp
    heap/MarkedSpace.cpp
    heap/MarkStack.cpp
    heap/WeakSet.cpp
    heap/WeakHandleOwner.cpp
    heap/WeakBlock.cpp

    heap/BlockAllocator.h
    heap/CardSet.h
    heap/ConservativeRoots.h
    heap/CopiedAllocator.h
    heap/CopiedBlock.h
    heap/CopiedSpace.h
    heap/CopiedSpaceInlineMethods.h
    heap/DFGCodeBlocks.h
    heap/GCAssertions.h
    heap/Handle.h
    heap/HandleSet.h
    heap/HandleStack.h
    heap/HandleTypes.h
    heap/Heap.h
    heap/HeapBlock.h
    heap/HeapRootVisitor.h
    heap/ListableHandler.h
    heap/Local.h
    heap/LocalScope.h
    heap/MachineStackMarker.h
    heap/MarkedAllocator.h
    heap/MarkedBlock.h
    heap/MarkedBlockSet.h
    heap/MarkedSpace.h
    heap/MarkStack.h
    heap/PassWeak.h
    heap/SlotVisitor.h
    heap/Strong.h
    heap/StrongInlines.h
    heap/TinyBloomFilter.h
    heap/UnconditionalFinalizer.h
    heap/VTableSpectrum.h
    heap/Weak.h
    heap/WeakBlock.h
    heap/WeakHandleOwner.h
    heap/WeakImpl.h
    heap/WeakReferenceHarvester.h
    heap/WeakSet.h
    heap/WeakSetInlines.h
    heap/WriteBarrierSupport.h

    debugger/Debugger.cpp
    debugger/DebuggerActivation.cpp
    debugger/DebuggerCallFrame.cpp

    debugger/Debugger.h
    debugger/DebuggerActivation.h
    debugger/DebuggerCallFrame.h

    interpreter/AbstractPC.cpp
    interpreter/CallFrame.cpp
    interpreter/Interpreter.cpp
    interpreter/RegisterFile.cpp

    interpreter/AbstractPC.h
    interpreter/CachedCall.h
    interpreter/CallFrame.h
    interpreter/CallFrameClosure.h
    interpreter/Interpreter.h
    interpreter/Register.h
    interpreter/RegisterFile.h

    jit/ExecutableAllocator.cpp
    jit/HostCallReturnValue.cpp
    jit/JITArithmetic32_64.cpp
    jit/JITArithmetic.cpp
    jit/JITCall32_64.cpp
    jit/JITCall.cpp
    jit/JIT.cpp
    jit/JITExceptions.cpp
    jit/JITOpcodes32_64.cpp
    jit/JITOpcodes.cpp
    jit/JITPropertyAccess32_64.cpp
    jit/JITPropertyAccess.cpp
    jit/JITStubs.cpp
    jit/ThunkGenerators.cpp

    jit/CompactJITCodeMap.h
    jit/ExecutableAllocator.h
    jit/HostCallReturnValue.h
    jit/JIT.h
    jit/JITCode.h
    jit/JITCompilationEffort.h
    jit/JITDriver.h
    jit/JITExceptions.h
    jit/JITInlineMethods.h
    jit/JITStubCall.h
    jit/JITStubs.h
    jit/JITWriteBarrier.h
    jit/JSInterfaceJIT.h
    jit/SpecializedThunkJIT.h
    jit/ThunkGenerators.h

    parser/Lexer.cpp
    parser/Nodes.cpp
    parser/Parser.cpp
    parser/ParserArena.cpp
    parser/SourceProviderCache.cpp

    parser/ASTBuilder.h
    parser/Lexer.h
    parser/NodeConstructors.h
    parser/NodeInfo.h
    parser/Nodes.h
    parser/Parser.h
    parser/ParserArena.h
    parser/ParserTokens.h
    parser/ResultType.h
    parser/SourceCode.h
    parser/SourceProvider.h
    parser/SourceProviderCache.h
    parser/SourceProviderCacheItem.h
    parser/SyntaxChecker.h

    profiler/Profile.cpp
    profiler/ProfileGenerator.cpp
    profiler/ProfileNode.cpp
    profiler/Profiler.cpp

    profiler/CallIdentifier.h
    profiler/Profile.h
    profiler/ProfileGenerator.h
    profiler/ProfileNode.h
    profiler/Profiler.h

    runtime/ArgList.cpp
    runtime/Arguments.cpp
    runtime/ArrayConstructor.cpp
    runtime/ArrayPrototype.cpp
    runtime/BooleanConstructor.cpp
    runtime/BooleanObject.cpp
    runtime/BooleanPrototype.cpp
    runtime/CallData.cpp
    runtime/CommonIdentifiers.cpp
    runtime/Completion.cpp
    runtime/ConstructData.cpp
    runtime/DateConstructor.cpp
    runtime/DateConversion.cpp
    runtime/DateInstance.cpp
    runtime/DatePrototype.cpp
    runtime/Error.cpp
    runtime/ErrorConstructor.cpp
    runtime/ErrorInstance.cpp
    runtime/ErrorPrototype.cpp
    runtime/ExceptionHelpers.cpp
    runtime/Executable.cpp
    runtime/FunctionConstructor.cpp
    runtime/FunctionPrototype.cpp
    runtime/GCActivityCallback.cpp
    runtime/GetterSetter.cpp
    runtime/Identifier.cpp
    runtime/InitializeThreading.cpp
    runtime/InternalFunction.cpp
    runtime/JSActivation.cpp
    runtime/JSAPIValueWrapper.cpp
    runtime/JSArray.cpp
    runtime/JSCell.cpp
    runtime/JSDateMath.cpp
    runtime/JSFunction.cpp
    runtime/JSBoundFunction.cpp
    runtime/JSGlobalData.cpp
    runtime/JSGlobalObject.cpp
    runtime/JSGlobalObjectFunctions.cpp
    runtime/JSGlobalThis.cpp
    runtime/JSLock.cpp
    runtime/JSNotAnObject.cpp
    runtime/JSObject.cpp
    runtime/JSONObject.cpp
    runtime/JSPropertyNameIterator.cpp
    runtime/JSStaticScopeObject.cpp
    runtime/JSString.cpp
    runtime/JSStringJoiner.cpp
    runtime/JSValue.cpp
    runtime/JSVariableObject.cpp
    runtime/JSWrapperObject.cpp
    runtime/LiteralParser.cpp
    runtime/Lookup.cpp
    runtime/MathObject.cpp
    runtime/NativeErrorConstructor.cpp
    runtime/NativeErrorPrototype.cpp
    runtime/NumberConstructor.cpp
    runtime/NumberObject.cpp
    runtime/NumberPrototype.cpp
    runtime/ObjectConstructor.cpp
    runtime/ObjectPrototype.cpp
    runtime/Operations.cpp
    runtime/Options.cpp
    runtime/PropertyDescriptor.cpp
    runtime/PropertyNameArray.cpp
    runtime/PropertySlot.cpp
    runtime/RegExp.cpp
    runtime/RegExpCache.cpp
    runtime/RegExpConstructor.cpp
    runtime/RegExpCachedResult.cpp
    runtime/RegExpMatchesArray.cpp
    runtime/RegExpObject.cpp
    runtime/RegExpPrototype.cpp
    runtime/ScopeChain.cpp
    runtime/SmallStrings.cpp
    runtime/StrictEvalActivation.cpp
    runtime/StringConstructor.cpp
    runtime/StringObject.cpp
    runtime/StringPrototype.cpp
    runtime/StringRecursionChecker.cpp
    runtime/Structure.cpp
    runtime/StructureChain.cpp
    runtime/TimeoutChecker.cpp
    runtime/UString.cpp

    runtime/ArgList.h
    runtime/Arguments.h
    runtime/ArrayConstructor.h
    runtime/ArrayPrototype.h
    runtime/BatchedTransitionOptimizer.h
    runtime/BigInteger.h
    runtime/BooleanConstructor.h
    runtime/BooleanObject.h
    runtime/BooleanPrototype.h
    runtime/CachedTranscendentalFunction.h
    runtime/CallData.h
    runtime/ClassInfo.h
    runtime/CodeSpecializationKind.h
    runtime/CommonIdentifiers.h
    runtime/CommonSlowPaths.h
    runtime/Completion.h
    runtime/ConstructData.h
    runtime/DateConstructor.h
    runtime/DateConversion.h
    runtime/DateInstance.h
    runtime/DateInstanceCache.h
    runtime/DatePrototype.h
    runtime/Error.h
    runtime/ErrorConstructor.h
    runtime/ErrorInstance.h
    runtime/ErrorPrototype.h
    runtime/ExceptionHelpers.h
    runtime/Executable.h
    runtime/ExecutionHarness.h
    runtime/FunctionConstructor.h
    runtime/FunctionPrototype.h
    runtime/GCActivityCallback.h
    runtime/GetterSetter.h
    runtime/Identifier.h
    runtime/InitializeThreading.h
    runtime/InternalFunction.h
    runtime/Intrinsic.h
    runtime/JSActivation.h
    runtime/JSAPIValueWrapper.h
    runtime/JSArray.h
    runtime/JSBoundFunction.h
    runtime/JSCell.h
    runtime/JSChunk.h
    runtime/JSDateMath.h
    runtime/JSExportMacros.h
    runtime/JSFunction.h
    runtime/JSGlobalData.h
    runtime/JSGlobalObject.h
    runtime/JSGlobalObjectFunctions.h
    runtime/JSGlobalThis.h
    runtime/JSLock.h
    runtime/JSNotAnObject.h
    runtime/JSObject.h
    runtime/JSONObject.h
    runtime/JSPropertyNameIterator.h
    runtime/JSStaticScopeObject.h
    runtime/JSString.h
    runtime/JSStringBuilder.h
    runtime/JSStringJoiner.h
    runtime/JSType.h
    runtime/JSTypeInfo.h
    runtime/JSValue.h
    runtime/JSValueInlineMethods.h
    runtime/JSVariableObject.h
    runtime/JSWrapperObject.h
    runtime/LiteralParser.h
    runtime/Lookup.h
    runtime/MatchResult.h
    runtime/MathObject.h
    runtime/MemoryStatistics.h
    runtime/NativeErrorConstructor.h
    runtime/NativeErrorPrototype.h
    runtime/NumberConstructor.h
    runtime/NumberObject.h
    runtime/NumberPrototype.h
    runtime/NumericStrings.h
    runtime/ObjectConstructor.h
    runtime/ObjectPrototype.h
    runtime/Operations.h
    runtime/Options.h
    runtime/PropertyDescriptor.h
    runtime/PropertyMapHashTable.h
    runtime/PropertyNameArray.h
    runtime/PropertySlot.h
    runtime/Protect.h
    runtime/PutPropertySlot.h
    runtime/RegExp.h
    runtime/RegExpCache.h
    runtime/RegExpCachedResult.h
    runtime/RegExpConstructor.h
    runtime/RegExpKey.h
    runtime/RegExpMatchesArray.h
    runtime/RegExpObject.h
    runtime/RegExpPrototype.h
    runtime/SamplingCounter.h
    runtime/ScopeChain.h
    runtime/ScopeChainMark.h
    runtime/SmallStrings.h
    runtime/StorageBarrier.h
    runtime/StrictEvalActivation.h
    runtime/StringConstructor.h
    runtime/StringObject.h
    runtime/StringPrototype.h
    runtime/StringRecursionChecker.h
    runtime/Structure.h
    runtime/StructureChain.h
    runtime/StructureTransitionTable.h
    runtime/SymbolTable.h
    runtime/Terminator.h
    runtime/TimeoutChecker.h
    runtime/Tracing.h
    runtime/Uint16WithFraction.h
    runtime/UString.h
    runtime/UStringBuilder.h
    runtime/UStringConcatenate.h
    runtime/WeakGCMap.h
    runtime/WeakRandom.h
    runtime/WriteBarrier.h

    tools/CodeProfile.cpp
    tools/CodeProfiling.cpp

    tools/CodeProfile.h
    tools/CodeProfiling.h
    tools/ProfileTreeNode.h
    tools/TieredMMapArray.h

    yarr/YarrCanonicalizeUCS2.cpp
    yarr/YarrPattern.cpp
    yarr/YarrInterpreter.cpp
    yarr/YarrJIT.cpp
    yarr/YarrSyntaxChecker.cpp

    yarr/Yarr.h
    yarr/YarrCanonicalizeUCS2.h
    yarr/YarrInterpreter.h
    yarr/YarrJIT.h
    yarr/YarrParser.h
    yarr/YarrPattern.h
    yarr/YarrSyntaxChecker.h
)

IF(GIBBON_SCRIPT_JSC_BACKDOOR) ### Added by Netflix
    LIST(APPEND JavaScriptCore_INCLUDE_DIRECTORIES "${JAVASCRIPTCORE_DIR}/netflix")
    LIST(APPEND JavaScriptCore_SOURCES netflix/NetflixJSCBackdoor.cpp)
ENDIF()

SET(JavaScriptCore_LUT_FILES
    runtime/ArrayConstructor.cpp
    runtime/ArrayPrototype.cpp
    runtime/BooleanPrototype.cpp
    runtime/DateConstructor.cpp
    runtime/DatePrototype.cpp
    runtime/ErrorPrototype.cpp
    runtime/JSGlobalObject.cpp
    runtime/JSONObject.cpp
    runtime/MathObject.cpp
    runtime/NumberConstructor.cpp
    runtime/NumberPrototype.cpp
    runtime/ObjectConstructor.cpp
    runtime/ObjectPrototype.cpp
    runtime/RegExpConstructor.cpp
    runtime/RegExpObject.cpp
    runtime/RegExpPrototype.cpp
    runtime/StringConstructor.cpp
    runtime/StringPrototype.cpp
)

SET(JavaScriptCore_LIBRARIES
    ${WTF_LIBRARY_NAME}
)


# GENERATOR 1-A: LUT creator
FOREACH (_file ${JavaScriptCore_LUT_FILES})
    GET_FILENAME_COMPONENT(_name ${_file} NAME_WE)
    GENERATE_HASH_LUT(${JAVASCRIPTCORE_DIR}/${_file} ${DERIVED_SOURCES_JAVASCRIPTCORE_DIR}/${_name}.lut.h)
    LIST(APPEND JavaScriptCore_HEADERS ${DERIVED_SOURCES_JAVASCRIPTCORE_DIR}/${_name}.lut.h)
ENDFOREACH ()


# GENERATOR 1-B: particular LUT creator (for 1 file only)
GENERATE_HASH_LUT(${JAVASCRIPTCORE_DIR}/parser/Keywords.table ${DERIVED_SOURCES_JAVASCRIPTCORE_DIR}/Lexer.lut.h MAIN_DEPENDENCY)
LIST(APPEND JavaScriptCore_HEADERS ${DERIVED_SOURCES_JAVASCRIPTCORE_DIR}/Lexer.lut.h)

#GENERATOR: "RegExpJitTables.h": tables used by Yarr
ADD_CUSTOM_COMMAND(
    OUTPUT ${DERIVED_SOURCES_JAVASCRIPTCORE_DIR}/RegExpJitTables.h
    MAIN_DEPENDENCY ${JAVASCRIPTCORE_DIR}/create_regex_tables
    COMMAND ${PYTHON_EXECUTABLE} ${JAVASCRIPTCORE_DIR}/create_regex_tables > ${DERIVED_SOURCES_JAVASCRIPTCORE_DIR}/RegExpJitTables.h
    VERBATIM)
ADD_SOURCE_DEPENDENCIES(${JAVASCRIPTCORE_DIR}/yarr/YarrPattern.cpp ${DERIVED_SOURCES_JAVASCRIPTCORE_DIR}/RegExpJitTables.h)


#GENERATOR: "KeywordLookup.h": keyword decision tree used by the lexer
ADD_CUSTOM_COMMAND(
    OUTPUT ${DERIVED_SOURCES_JAVASCRIPTCORE_DIR}/KeywordLookup.h
    MAIN_DEPENDENCY ${JAVASCRIPTCORE_DIR}/KeywordLookupGenerator.py
    COMMAND ${PYTHON_EXECUTABLE} ${JAVASCRIPTCORE_DIR}/KeywordLookupGenerator.py ${JAVASCRIPTCORE_DIR}/parser/Keywords.table > ${DERIVED_SOURCES_JAVASCRIPTCORE_DIR}/KeywordLookup.h
    VERBATIM)
ADD_SOURCE_DEPENDENCIES(${JAVASCRIPTCORE_DIR}/parser/Lexer.cpp ${DERIVED_SOURCES_JAVASCRIPTCORE_DIR}/KeywordLookup.h)

IF (WTF_CPU_ARM)
    LIST(APPEND JavaScriptCore_SOURCES
        assembler/ARMAssembler.cpp
        assembler/ARMv7Assembler.cpp
        assembler/MacroAssemblerARM.cpp
    )
ELSEIF (WTF_CPU_MIPS)
ELSEIF (WTF_CPU_X86)
ELSEIF (WTF_CPU_X86_64)
ELSEIF (WTF_CPU_PPC)
ELSE ()
    MESSAGE(FATAL_ERROR "Unknown CPU")
ENDIF ()

WEBKIT_INCLUDE_CONFIG_FILES_IF_EXISTS()

IF(JAVASCRIPTCORE_BUILD_JSC) ### Added by Netflix
    ADD_SUBDIRECTORY(shell)
ENDIF()

WEBKIT_WRAP_SOURCELIST(${JavaScriptCore_SOURCES})
INCLUDE_DIRECTORIES(${JavaScriptCore_INCLUDE_DIRECTORIES})
ADD_DEFINITIONS(-DBUILDING_JavaScriptCore)
ADD_LIBRARY(${JavaScriptCore_LIBRARY_NAME} ${JavaScriptCore_LIBRARY_TYPE} ${JavaScriptCore_HEADERS} ${JavaScriptCore_SOURCES})
TARGET_LINK_LIBRARIES(${JavaScriptCore_LIBRARY_NAME} ${JavaScriptCore_LIBRARIES})
SET_TARGET_PROPERTIES(${JavaScriptCore_LIBRARY_NAME} PROPERTIES FOLDER "JavaScriptCore")
SET_TARGET_PROPERTIES(${JavaScriptCore_LIBRARY_NAME} PROPERTIES LINK_INTERFACE_LIBRARIES "")

IF (JavaScriptCore_LINK_FLAGS)
    ADD_TARGET_PROPERTIES(${JavaScriptCore_LIBRARY_NAME} LINK_FLAGS "${JavaScriptCore_LINK_FLAGS}")
ENDIF ()

IF (SHARED_CORE)
    SET_TARGET_PROPERTIES(${JavaScriptCore_LIBRARY_NAME} PROPERTIES VERSION ${PROJECT_VERSION} SOVERSION ${PROJECT_VERSION_MAJOR})
    INSTALL(TARGETS ${JavaScriptCore_LIBRARY_NAME} DESTINATION "${LIB_INSTALL_DIR}")
ENDIF ()

