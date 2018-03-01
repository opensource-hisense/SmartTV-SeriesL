#ifndef FileLineTracking_h
#define FileLineTracking_h


#ifdef NRDP_USE_JSC_FILE_LINE_TRACKING
namespace WTF {
    struct FileLineAllocTag
    {
      int trackIndex_;
    };

    int fileLineGetStringId(const char *s);
    int fileLineTrackMalloc(int size); // returns tracking index
    void fileLineTrackFree(int index);

    // Implemented in Interpreter.cpp
    int fileLineQueryInterpreter(char *fileLineOutput, int fileLineOutputSize); // returns non-zero on success
}
#endif

extern "C" {
    // Always available due to DLL exporting issues
    void JSFileLineSetLogFilename(const char *filename);
}

#endif
