#include "FileLineTracking.h"

#include <string.h>
#include <stdio.h>

#ifndef NRDP_USE_JSC_FILE_LINE_TRACKING
extern "C" {
    void JSFileLineSetLogFilename(const char *filename) {}
}
#else
static char sLogFilename[512] = { 0 };
extern "C" {
    void JSFileLineSetLogFilename(const char *filename)
    {
        strncpy(sLogFilename, filename, sizeof(sLogFilename));
        sLogFilename[ sizeof(sLogFilename) - 1 ] = 0;
    }
}
#endif

#ifdef NRDP_USE_JSC_FILE_LINE_TRACKING

#include <string>
#include <map>

typedef std::map<std::string, int> StringMap;
static StringMap sStringTable;
static int sNextStringID = 0;
static FILE *sLogFile = NULL;

// TODO: Mutex protect calls? They technically should only ever be called from the JSC thread, but perhaps that isn't true?

namespace WTF {

static void fileLineLogNewString(int index, const char *s);

bool fileLineOpen()
{
    if(sLogFile)
        return true;

    if(sLogFilename[0] == 0)
        return false;

    static char *filePrefix = "file://";
    char *s = strstr(sLogFilename, filePrefix);
    if(s == sLogFilename)
    {
        s += strlen(filePrefix);
    }
    else
    {
        s = sLogFilename;
    }

    sLogFile = fopen(s, "w");
    return (sLogFile != NULL);
}

int fileLineGetStringId(const char *s)
{
    if(fileLineOpen())
    {
        std::map<std::string, int>::iterator it = sStringTable.find(s);
        if(it == sStringTable.end())
        {
            // First time we've seen this string. Give it a fresh ID.
            int id = ++sNextStringID;
            sStringTable[s] = id;
            fileLineLogNewString(id, s);
            return id;
        }
        else
        {
            // Return this string's ID.
            return it->second;
        }
    }
    return 0;
}

static void fileLineLogNewString(int index, const char *s)
{
    if(!fileLineOpen())
        return;

    fprintf(sLogFile, "S %d %s\n", index, s);
}

static int fileLineLogAlloc(const char *stack, int size) // returns id
{
    if(stack[0] == 0)
        return 0;

    if(!fileLineOpen())
        return 0;

    static int nextID = 0;
    nextID++;
    fprintf(sLogFile, "A %d %d %s\n", nextID, size, stack);
    return nextID;
}

static void fileLineLogFree(int id)
{
    if(id == 0)
        return;

    if(!fileLineOpen())
        return;

    fprintf(sLogFile, "F %d\n", id);
}

int fileLineTrackMalloc(int size)
{
    if(fileLineOpen())
    {
        // Request the top of the JSC stack
        char fileLine[4096];
        if(fileLineQueryInterpreter(fileLine, sizeof(fileLine)))
        {
            return fileLineLogAlloc(fileLine, size);
        }
    }
    return 0;
}

void fileLineTrackFree(int index)
{
    if(fileLineOpen())
    {
        fileLineLogFree(index);
    }
}

} // namespace WTF

#endif
