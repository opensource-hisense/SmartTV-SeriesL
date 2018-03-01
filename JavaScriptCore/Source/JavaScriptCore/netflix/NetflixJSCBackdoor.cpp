#include <config.h>

#include <API/APICast.h>
#include <API/APIShims.h>
#include <API/JSBase.h>
#include <API/JSContextRef.h>
#include <API/JSStringRef.h>
#include <API/JSValueRef.h>
#include <API/OpaqueJSString.h>
#include <assert.h>
#include <bytecode/SamplingTool.h>
#include <debugger/Debugger.h>
#include <debugger/DebuggerCallFrame.h>
#include <interpreter/Interpreter.h>
#include <parser/SourceProvider.h>
#include <profiler/Profiler.h>
#include <runtime/InitializeThreading.h>
#include <runtime/UStringBuilder.h>
#include <runtime/JSFunction.h>
#include <runtime/JSNotAnObject.h>
#include <runtime/JSObject.h>
#include <runtime/JSActivation.h>
#include <runtime/Structure.h>
#include <stdarg.h>
#include <wtf/HashMap.h>
#include <wtf/MainThread.h>
#include <wtf/FastMalloc.h>
#include <wtf/Vector.h>
#include <wtf/text/StringBuilder.h>
#include <wtf/unicode/UTF8.h>
#include <wtf/OSAllocator.h>

// #define DEBUGGER_DEBUG

typedef void (*TimeoutCallback)(void *, const char *);

class NetflixGlobalObject : public JSC::JSGlobalObject
{
public:
    typedef JSC::JSGlobalObject Base;

    NetflixGlobalObject(JSC::JSGlobalData&, JSC::Structure*);

    static NetflixGlobalObject* create(JSC::JSGlobalData& globalData, JSC::Structure* structure)
    {
        NetflixGlobalObject* object = new (NotNull, JSC::allocateCell<NetflixGlobalObject>(globalData.heap)) NetflixGlobalObject(globalData, structure);
        object->finishCreation(globalData);
        return object;
    }

    static const JSC::ClassInfo s_info;
    static const JSC::GlobalObjectMethodTable s_globalObjectMethodTable;

    static JSC::Structure* createStructure(JSC::JSGlobalData& globalData, JSC::JSValue prototype)
    {
        return JSC::Structure::create(globalData, 0, prototype, JSC::TypeInfo(JSC::GlobalObjectType, StructureFlags), &s_info);
    }

    static void dumpStackTrace(const JSC::JSGlobalObject* obj);
    static void setTimeoutCallback(unsigned int in, TimeoutCallback cb, void *userData)
    {
        timeoutInterval = in;
        timeoutCallback = cb;
        timeoutUserData = userData;
    }

    static bool shouldInterruptScript(const JSC::JSGlobalObject* obj) { dumpStackTrace(obj); return false; }
    static bool supportsProfiling(const JSGlobalObject*) { return profilerEnabled; }

    static void setProfilerEnabled(bool enabled) { profilerEnabled = enabled; }

    static void setUseBackdoorGarbageCollect(bool use) { backdoorGarbageCollect = use; }
    static bool useBackdoorGarbageCollect() { return backdoorGarbageCollect; }

private:
    static unsigned int timeoutInterval;
    static TimeoutCallback timeoutCallback;
    static void *timeoutUserData;
    static bool profilerEnabled;
    static bool backdoorGarbageCollect;
};

unsigned int NetflixGlobalObject::timeoutInterval = 0;
TimeoutCallback NetflixGlobalObject::timeoutCallback = 0;
void *NetflixGlobalObject::timeoutUserData = 0;
bool NetflixGlobalObject::profilerEnabled = false;
bool NetflixGlobalObject::backdoorGarbageCollect = false;
const JSC::ClassInfo NetflixGlobalObject::s_info = { "GlobalObject", &JSC::JSGlobalObject::s_info, 0, JSC::ExecState::globalObjectTable, CREATE_METHOD_TABLE(NetflixGlobalObject) };
const JSC::GlobalObjectMethodTable NetflixGlobalObject::s_globalObjectMethodTable = { &allowsAccessFrom, &supportsProfiling, &supportsRichSourceInfo, &shouldInterruptScript };

NetflixGlobalObject::NetflixGlobalObject(JSC::JSGlobalData& globalData, JSC::Structure* structure)
    : JSGlobalObject(globalData, structure, &s_globalObjectMethodTable)
{
}

typedef void (*BreakpointCallback)(intptr_t sourceID, int line, int column, int reason, void* userData);
enum { Breakpoint, Exception, Step, CallstackSize };
extern "C" JS_EXPORT JSStringRef Netflix_JSStringCreate(const char* string, int length);

static JSC::SourceProvider* idToSourceProvider(intptr_t sourceID)
{
    // ### horrible hack
    return reinterpret_cast<JSC::SourceProvider*>(sourceID);
}

namespace WTF
{
// Make JSC::UString work in a HashMap
template<> struct HashTraits<JSC::UString> : GenericHashTraits<JSC::UString>
{
    static const bool hasIsEmptyValueFunction = true;
    static const bool emptyValueIsZero = true;
    static void constructDeletedValue(JSC::UString& slot) { new (NotNull, &slot) JSC::UString(static_cast<char*>(0), 0); }
    static bool isDeletedValue(const JSC::UString& value) { return value.isNull(); }
    static bool isEmptyValue(const JSC::UString& str) { return str.isEmpty(); }
};
}

struct NetflixBreakpoint
{
    JSC::UString file;
    intptr_t sourceID;
    int line;
    JSC::UString condition;

    NetflixBreakpoint(const JSC::UString& f, int l)
        : file(f), sourceID(0), line(l)
    {
    }
    NetflixBreakpoint(intptr_t s, int l)
        : sourceID(s), line(l)
    {
    }
    NetflixBreakpoint(const JSC::UString& f, int l, const JSC::UString& c)
        : file(f), sourceID(0), line(l), condition(c)
    {
    }

    bool operator==(const NetflixBreakpoint& other) const
    {
        return (sourceID == other.sourceID || file == other.file) && line == other.line;
    }
};

class NetflixStack
{
public:
    void push(const JSC::DebuggerCallFrame& frame, intptr_t sourceID, int lineNumber, int columnNumber);
    void update(const JSC::DebuggerCallFrame& frame, intptr_t sourceID, int lineNumber, int columnNumber);
    void pop();

    bool isEmpty() const { return mCallFrames.isEmpty(); }
    size_t size() const { return mCallFrames.size(); }

    void clear() { mCallFrames.clear(); mFrameNumber = 1; }

    const JSC::DebuggerCallFrame& currentFrame() const { return frameAt(mFrameNumber); }
    const JSC::DebuggerCallFrame& frameAt(size_t frame) const { return mCallFrames.at(frame - 1).frame; }
    JSC::DebuggerCallFrame& currentFrame() { return frameAt(mFrameNumber); }
    JSC::DebuggerCallFrame& frameAt(size_t frame) { return mCallFrames.at(frame - 1).frame; }

    intptr_t currentSourceID() const { return sourceIDAt(mFrameNumber); }
    intptr_t sourceIDAt(size_t frame) const { return mCallFrames.at(frame - 1).sourceID; }
    int currentLineNumber() const { return lineNumberAt(mFrameNumber); }
    int lineNumberAt(size_t frame) const { return mCallFrames.at(frame - 1).lineNumber; }
    int columnNumberAt(size_t frame) const { return mCallFrames.at(frame - 1).columnNumber; }

    void setAboutToStepOut(bool value) { mCallFrames.first().aboutToStepOut = value; }

    enum StepState {
        StepNone = 0x0,
        StepOver = 0x1,
        StepInto = 0x2|StepOver // step into should also be step over
    };
    int stepState() const;
    void setStepState(int s);
    void setCallerStepState(int s);
    void clearStepStates();

    void setFrameNumber(size_t frame) { if (frame > 0 && (frame == 1 || frame <= mCallFrames.size())) mFrameNumber = frame; }
    size_t frameNumber() const { return mFrameNumber; }

private:
    struct Entry
    {
        JSC::DebuggerCallFrame frame;
        intptr_t sourceID;
        int lineNumber, columnNumber;
        int state;
        bool aboutToStepOut;
    };
    WTF::Vector<Entry> mCallFrames;
    size_t mFrameNumber;
};

void NetflixStack::push(const JSC::DebuggerCallFrame& frame, intptr_t sourceID, int lineNumber, int columnNumber)
{
    Entry e = { frame, sourceID, lineNumber, columnNumber, StepNone, false };
    mCallFrames.prepend(e);
    mFrameNumber = 1;
}

void NetflixStack::update(const JSC::DebuggerCallFrame& frame, intptr_t sourceID, int lineNumber, int columnNumber)
{
    if (mCallFrames.isEmpty())
        push(frame, sourceID, lineNumber, columnNumber);
    else {
        Entry& e = mCallFrames.first();
        e.frame = frame;
        e.sourceID = sourceID;
        e.lineNumber = lineNumber;
        e.columnNumber = columnNumber;
    }
}

void NetflixStack::pop()
{
    mCallFrames.remove(0);
}

int NetflixStack::stepState() const
{
    WTF::Vector<Entry>::const_iterator entry = mCallFrames.begin();
    return entry->state;
}

void NetflixStack::setStepState(int s)
{
    WTF::Vector<Entry>::iterator entry = mCallFrames.begin();
    if (entry->aboutToStepOut)
        ++entry;
    entry->state = s;
}

void NetflixStack::setCallerStepState(int s)
{
    WTF::Vector<Entry>::iterator entry = mCallFrames.begin();
    if (entry->aboutToStepOut)
        ++entry;
    ++entry;
    entry->state = s;
}

void NetflixStack::clearStepStates()
{
    WTF::Vector<Entry>::iterator it = mCallFrames.begin();
    const WTF::Vector<Entry>::const_iterator end = mCallFrames.end();
    while (it != end) {
        it->state = StepNone;
        ++it;
    }
}

template <typename CharType>
static WTF::CString quoteString(const CharType* data, int length)
{
    JSC::UStringBuilder builder;
    for (int i = 0; i < length; ++i) {
        int start = i;
        while (i < length && (data[i] > 0x1F && data[i] != '"' && data[i] != '\\'))
            ++i;
        builder.append(data + start, i - start);
        if (i >= length)
            break;
        switch (data[i]) {
        case '\t':
            builder.append('\\');
            builder.append('t');
            break;
        case '\r':
            builder.append('\\');
            builder.append('r');
            break;
        case '\n':
            builder.append('\\');
            builder.append('n');
            break;
        case '\f':
            builder.append('\\');
            builder.append('f');
            break;
        case '\b':
            builder.append('\\');
            builder.append('b');
            break;
        case '"':
            builder.append('\\');
            builder.append('"');
            break;
        case '\\':
            builder.append('\\');
            builder.append('\\');
            break;
        default:
            static const char hexDigits[] = "0123456789abcdef";
            UChar ch = data[i];
            LChar hex[] = { '\\', 'u', hexDigits[(ch >> 12) & 0xF], hexDigits[(ch >> 8) & 0xF], hexDigits[(ch >> 4) & 0xF], hexDigits[ch & 0xF] };
            builder.append(hex, WTF_ARRAY_LENGTH(hex));
            break;
        }
    }
    return builder.toUString().utf8();
}

static inline WTF::CString quoteString(const JSC::UString& str)
{
    if (!str.impl())
        return WTF::CString();
    if (str.is8Bit())
        return quoteString<LChar>(str.characters8(), str.length());
    return quoteString<UChar>(str.characters16(), str.length());
}

class NetflixDebugger : public JSC::Debugger
{
public:
    WTF::HashMap<JSC::UString, JSC::SourceProvider*> providers;
    WTF::Vector<NetflixBreakpoint> breaks;
    BreakpointCallback callback;
    void* userData;
    NetflixStack callstack;
    bool inDebug;
    int attachRef;
    enum ExceptionBreak { BreakNone, BreakAll, BreakUncaught };
    ExceptionBreak breakOnException;
    bool pause;

    enum { MaxCallstackThreshold = 1000 };

    static int nextId;
    struct State
    {
        WTF::Vector<NetflixBreakpoint> breaks;
        ExceptionBreak breakOnException;
    };

    static WTF::HashMap<int, State> savedState;

    struct DebugScope
    {
        DebugScope(NetflixDebugger* d) : debugger(d) { alreadyInDebug = debugger->inDebug; debugger->inDebug = true; }
        ~DebugScope() { if (!alreadyInDebug) debugger->inDebug = false; }

        NetflixDebugger* debugger;
        bool alreadyInDebug;
    };

    static NetflixDebugger* get(JSC::JSGlobalObject* global);
    static void reset(JSC::JSGlobalObject* global);

    int saveBreakpoints()
    {
        int id = ++nextId;
        if (id == -1) // avoid returning -1
            id = ++nextId;
        State state = { breaks, breakOnException };
        savedState.add(id, state);
        return id;
    }

    void restoreBreakpoints(int id)
    {
        WTF::HashMap<int, State>::iterator it = savedState.find(id);
        if (it == savedState.end())
            return;
        breaks = it->second.breaks;
        breakOnException = it->second.breakOnException;
        resolveAll(true);
        savedState.remove(it);
    }

    virtual void sourceParsed(JSC::ExecState* /*exec*/, JSC::SourceProvider* provider,
                              int /*errorLineNumber*/, const JSC::UString& /*errorMessage*/)
    {
        DebugScope scope(this);
        if (scope.alreadyInDebug)
            return;
        if (!provider->url().isEmpty())
            providers.add(provider->url(), provider);
        resolveAll();
        //printf("sourceParsed %p %p %d %s\n",
        //       exec, provider, errorLineNumber, quoteString(errorMessage).data());
    }

    virtual void exception(const JSC::DebuggerCallFrame& frame, intptr_t sourceID, int lineNumber, int columnNumber, bool hasHandler)
    {
        DebugScope scope(this);
        if (scope.alreadyInDebug)
            return;
#ifdef DEBUGGER_DEBUG
        printf("exception\n");
#endif
        callstack.update(frame, sourceID, lineNumber, columnNumber);
        if (breakOnException == BreakAll || (!hasHandler && breakOnException == BreakUncaught))
            breakHere(sourceID, lineNumber, columnNumber, Exception);
        else
            handleBreakpoint(sourceID, lineNumber, columnNumber);
    }

    virtual void atStatement(const JSC::DebuggerCallFrame& frame, intptr_t sourceID, int lineNumber, int columnNumber)
    {
        DebugScope scope(this);
        if (scope.alreadyInDebug)
            return;
#ifdef DEBUGGER_DEBUG
        printf("atStatement\n");
#endif
        callstack.update(frame, sourceID, lineNumber, columnNumber);
        handleBreakpoint(sourceID, lineNumber, columnNumber);
    }

    virtual void callEvent(const JSC::DebuggerCallFrame& frame, intptr_t sourceID, int lineNumber, int columnNumber)
    {
        DebugScope scope(this);
        if (scope.alreadyInDebug)
            return;
#ifdef DEBUGGER_DEBUG
        printf("callEvent\n");
#endif
        if (callstack.size() == MaxCallstackThreshold)
            breakHere(sourceID, lineNumber, columnNumber, CallstackSize);

        const int state = !callstack.isEmpty() ? callstack.stepState() : 0;
        callstack.push(frame, sourceID, lineNumber, columnNumber);
        if (state == NetflixStack::StepInto)
            callstack.setStepState(NetflixStack::StepOver);
        handleBreakpoint(sourceID, lineNumber, columnNumber);
    }

    virtual void returnEvent(const JSC::DebuggerCallFrame& frame, intptr_t sourceID, int lineNumber, int columnNumber)
    {
        DebugScope scope(this);
        if (scope.alreadyInDebug)
            return;
#ifdef DEBUGGER_DEBUG
        printf("returnEvent\n");
#endif
        callstack.update(frame, sourceID, lineNumber, columnNumber);
        callstack.setAboutToStepOut(true);
        handleBreakpoint(sourceID, lineNumber, columnNumber);
        callstack.pop();
    }

    virtual void willExecuteProgram(const JSC::DebuggerCallFrame& frame, intptr_t sourceID, int lineNumber, int columnNumber)
    {
        DebugScope scope(this);
        if (scope.alreadyInDebug)
            return;
#ifdef DEBUGGER_DEBUG
        printf("willExecuteProgram\n");
#endif
        if (callstack.size() == MaxCallstackThreshold)
            breakHere(sourceID, lineNumber, columnNumber, CallstackSize);

        const int state = !callstack.isEmpty() ? callstack.stepState() : 0;
        callstack.push(frame, sourceID, lineNumber, columnNumber);
        if (state == NetflixStack::StepInto)
            callstack.setStepState(NetflixStack::StepOver);
        handleBreakpoint(sourceID, lineNumber, columnNumber);
    }

    virtual void didExecuteProgram(const JSC::DebuggerCallFrame& frame, intptr_t sourceID, int lineNumber, int columnNumber)
    {
        DebugScope scope(this);
        if (scope.alreadyInDebug)
            return;
#ifdef DEBUGGER_DEBUG
        printf("didExecuteProgram\n");
#endif
        callstack.update(frame, sourceID, lineNumber, columnNumber);
        callstack.setAboutToStepOut(true);
        handleBreakpoint(sourceID, lineNumber, columnNumber);
        callstack.pop();
    }

    virtual void didReachBreakpoint(const JSC::DebuggerCallFrame& frame, intptr_t sourceID, int lineNumber, int columnNumber)
    {
        DebugScope scope(this);
        if (scope.alreadyInDebug)
            return;
#ifdef DEBUGGER_DEBUG
        printf("didReachBreakpoint\n");
#endif
        callstack.update(frame, sourceID, lineNumber, columnNumber);
        handleBreakpoint(sourceID, lineNumber, BreakAlways);
    }

    void breakHere(intptr_t sourceID, int lineNumber, int columnNumber, int reason);
    enum BreakMode { BreakIfBreakpoint, BreakAlways };
    void handleBreakpoint(intptr_t sourceID, int lineNumber, int columnNumber, BreakMode mode = BreakIfBreakpoint);
    bool hasBreakpoint(intptr_t sourceID, int lineNumber, int columnNumber, JSC::UString** condition);
    void resolve(NetflixBreakpoint* point, bool force = false);
    void resolveAll(bool force = false);
    void pauseOnNextStatement();

private:
    NetflixDebugger()
        : callback(0), userData(0), inDebug(false), attachRef(0), breakOnException(BreakNone), pause(false)
    {
        callstack.setFrameNumber(0);
    }

    static WTF::HashMap<JSC::JSGlobalObject*, NetflixDebugger*> debuggers;
};

WTF::HashMap<JSC::JSGlobalObject*, NetflixDebugger*> NetflixDebugger::debuggers;
int NetflixDebugger::nextId = 0;
WTF::HashMap<int, NetflixDebugger::State> NetflixDebugger::savedState;

void NetflixDebugger::reset(JSC::JSGlobalObject* global)
{
    WTF::HashMap<JSC::JSGlobalObject*, NetflixDebugger*>::iterator it = debuggers.find(global);
    if (it != debuggers.end()) {
        if (it->second->attachRef) {
            it->second->detach(global);
            it->second->recompileAllJSFunctions(&global->globalData());
        }
        delete it->second;
        debuggers.remove(it);
    }
}

NetflixDebugger* NetflixDebugger::get(JSC::JSGlobalObject* global)
{
    WTF::HashMap<JSC::JSGlobalObject*, NetflixDebugger*>::const_iterator it = debuggers.find(global);
    if (it != debuggers.end())
        return it->second;
    NetflixDebugger* debugger = new NetflixDebugger;
    debuggers.add(global, debugger);
    return debugger;
}

void NetflixDebugger::pauseOnNextStatement()
{
    pause = true;
}

void NetflixDebugger::resolve(NetflixBreakpoint* point, bool force)
{
    if (point->sourceID && !force)
        return;
    WTF::HashMap<JSC::UString, JSC::SourceProvider*>::const_iterator it = providers.find(point->file);
    if (it == providers.end()) {
        if (force)
            point->sourceID = 0;
        return;
    }
    point->sourceID = it->second->asID();
}

void NetflixDebugger::resolveAll(bool force)
{
    WTF::Vector<NetflixBreakpoint>::iterator it = breaks.begin();
    const WTF::Vector<NetflixBreakpoint>::const_iterator end = breaks.end();
    while (it != end) {
        if (force || !it->sourceID) {
            resolve(&(*it), force);
        }
        ++it;
    }
}

bool NetflixDebugger::hasBreakpoint(intptr_t sourceID, int line, int column, JSC::UString** condition)
{
    NetflixBreakpoint point(sourceID, line);
    const size_t pos = breaks.find(point);
    if (pos == WTF::notFound) {
        if (condition)
            *condition = 0;
        return false;
    }
    if (condition) {
        if (!breaks.at(pos).condition.isEmpty())
            *condition = &breaks.at(pos).condition;
        else
            *condition = 0;
    }
    return true;
}

void NetflixDebugger::breakHere(intptr_t sourceID, int line, int column, int reason)
{
    if (callback)
        callback(sourceID, line, column, reason, userData);
}

void NetflixDebugger::handleBreakpoint(intptr_t sourceID, int line, int column, BreakMode mode)
{
    bool brk = false;
    JSC::UString* condition;
    int reason = Breakpoint;
    if (mode == BreakAlways || hasBreakpoint(sourceID, line, column, &condition))
        brk = true;
    else if (callstack.stepState() & NetflixStack::StepOver) {
        reason = Step;
        callstack.setStepState(NetflixStack::StepNone);
        brk = true;
    }
    if (brk && condition) {
        JSC::JSValue exc;
        JSC::JSValue val = callstack.currentFrame().evaluate(*condition, exc);
        if (!exc.isEmpty()) // a non-empty exception means we didn't hit the condition
            brk = false;
        else {
            if (val.isEmpty() || val.isNull() || val.isUndefined() || val.isFalse()
                || (val.isNumber() && val.asNumber() == 0.))
                brk = false;
        }
    }
    if (pause) {
        pause = false;
        brk = true;
    }
    if (brk)
        breakHere(sourceID, line, column, reason);
}

class NetflixForEachCell : public JSC::MarkedBlock::VoidFunctor
{
public:
    struct Block
    {
        size_t cellSize, blockSize, blockCapacity;
        WTF::Vector<JSC::JSCell*> cells;

        Block(size_t cs, size_t bs, size_t bc)
            : cellSize(cs), blockSize(bs), blockCapacity(bc)
        {
        }
    };
    WTF::Vector<Block> blocks;

    void operator()(JSC::JSCell* cell)
    {
        blocks.last().cells.append(cell);
    }

    static size_t currentCellSize;
};

size_t NetflixForEachCell::currentCellSize = 0;

class NetflixForEachBlock : public JSC::MarkedBlock::VoidFunctor
{
public:
    NetflixForEachCell foreachCell;

    void operator()(JSC::MarkedBlock* block)
    {
        foreachCell.blocks.append(NetflixForEachCell::Block(block->cellSize(), block->size(), block->capacity()));
        block->forEachCell<NetflixForEachCell>(foreachCell);
    }
};

typedef void (*DumpCallback)(void *userData, const char *fmt, ...); // duplicated in ScriptEngineJSC

static void dumpValue(JSC::ExecState* exec, void *userData, DumpCallback cb, const JSC::JSValue& val, int indent);
static void dumpCell(JSC::ExecState* exec, void *userData, DumpCallback cb, const JSC::JSCell* cell, int indent);
static void dumpObject(JSC::ExecState* exec, void *userData, DumpCallback cb, const JSC::JSObject* obj, int indent)
{
    JSC::UString className = JSC::JSObject::className(obj);
    cb(userData, "{\n");
    indent += 4;
    cb(userData, "%*s\"type\": \"Object\", \"name\": \"%s\", \"id\": \"%p\",\n", indent, "", quoteString(className).data(), obj);
    cb(userData, "%*s\"object\": {\n", indent, "");
    JSC::PropertyNameArray names(exec);
    JSC::JSObject::getPropertyNames(const_cast<JSC::JSObject*>(obj), exec, names, JSC::ExcludeDontEnumProperties);
    JSC::PropertyNameArray::const_iterator it = names.begin();
    const JSC::PropertyNameArray::const_iterator end = names.end();
    while (it != end) {
        const JSC::Identifier& ident = *it;
        const bool own = obj->hasOwnProperty(exec, ident);
        indent += 4;
        cb(userData, "%*s\"%s\": {\"own\": %s, \"value\": ", indent, "",
           quoteString(ident.ustring()).data(), own ? "true" : "false");

        JSC::JSValue value = JSC::jsUndefined();
        {
            JSC::PropertySlot slot(obj);
            if(const_cast<JSC::JSObject*>(obj)->getPropertySlot(exec, ident, slot)) {
                JSC::PropertySlot::GetValueFunc func = slot.customGetter();
                if(func != INDEX_GETTER_MARKER && func != GETTER_FUNCTION_MARKER) {
                    value = slot.getValue(exec, ident);
                    if (exec->hadException())
                        exec->clearException();
                }
            }
        }
        dumpValue(exec, userData, cb, value, indent);
        indent -= 4;
        ++it;
        if (it != end)
            cb(userData, "},\n");
        else
            cb(userData, "}\n");
    }
    cb(userData, "%*s}\n", indent, "");
    indent -= 4;
    cb(userData, "%*s}\n", indent, "");
}

static void dumpFunction(JSC::ExecState* exec, void *userData, DumpCallback cb, JSC::JSFunction* func, int /*indent*/)
{
    const JSC::SourceCode* code = func->sourceCode();
    if (!code || code->isNull()) {
        cb(userData, "{ \"type\": \"Function\", \"id\": \"%p\" }\n", func);
        return;
    }

    JSC::JSActivation* act = 0;

    const JSC::UString name = func->calculatedDisplayName(exec);
    const JSC::UString file = code->provider()->url();

    cb(userData, "{ \"type\": \"Function\", \"id\": \"%p\", \"name\": \"%s\", \"file\": \"%s\", \"start\": %d, \"end\": %d",
       func, quoteString(name).data(), quoteString(file).data(), code->startOffset(), code->endOffset());
    if (!func->isHostFunction()) {
        JSC::ScopeChainNode* chain = func->scopeUnchecked();
        // find the first activation in the chain
        JSC::ScopeChainIterator it = chain->begin();
        JSC::ScopeChainIterator end = chain->end();
        if(it != end) {
            bool found = false;
            while(it != end) {
                if ((*it)->inherits(&JSC::JSActivation::s_info)) {
                    if(!found) {
                        cb(userData, ", \"activations\": [ \"%p\"", static_cast<JSC::JSActivation*>(it->get()));
                        found = true;
                    } else {
                        cb(userData, ", \"%p\"", static_cast<JSC::JSActivation*>(it->get()));
                    }
                }
                ++it;
            }
            if(found)
                cb(userData, " ]");
        }
    }
    cb(userData, " }\n");
}

static void dumpValue(JSC::ExecState* exec, void *userData, DumpCallback cb, const JSC::JSValue& val, int indent)
{
    if (val.isInt32()) {
        cb(userData, "%d", val.asInt32());
    } else if (val.isUInt32()) {
        cb(userData, "%u", val.asUInt32());
    } else if (val.isDouble()) {
        const double dbl = val.asDouble();
        if (isnan(dbl) || isinf(dbl)) {
            cb(userData, "null");
        } else {
            cb(userData, "%f", dbl);
        }
    } else if (val.isTrue()) {
        cb(userData, "true");
    } else if (val.isFalse()) {
        cb(userData, "false");
    } else if (val.isEmpty()) {
        cb(userData, "{ \"type\": \"Empty\" }");
    } else if (val.isUndefined() || val.isNull()) {
        // Print 'null' for undefined, JSON only allows 'null'
        cb(userData, "null");
    } else if (val.isCell() && val.asCell()->inherits(&JSC::JSFunction::s_info)) {
        cb(userData, "{ \"type\": \"Function\", \"id\": \"%p\" }", val.asCell());
    } else if (val.isString()) {
        cb(userData, "{ \"type\": \"String\", \"id\": \"%p\" }", val.asCell());
    } else if (val.isObject()) {
        cb(userData, "{ \"type\": \"Object\", \"id\": \"%p\" }", val.asCell());
    } else if (val.isCell()) {
        dumpCell(exec, userData, cb, val.asCell(), indent);
    } else {
        cb(userData, "{ \"type\": \"Unknown value\", \"descr\": \"%s\" }", const_cast<JSC::JSValue*>(&val)->description());
    }
}

static void dumpCell(JSC::ExecState* exec, void *userData, DumpCallback cb, const JSC::JSCell* cell, int indent)
{
    cb(userData, "%*s{ \"type\": \"Cell\", \"id\": \"%p\",\n", indent, "", cell, NetflixForEachCell::currentCellSize);
    cb(userData, "%*s\"data\": ", indent + 2, "");
    indent += 4;
    if (cell->isString()) {
        JSC::UString str = cell->getString(exec);
        cb(userData, "{ \"type\": \"String\", \"id\": \"%p\", \"data\": \"%s\" }\n", cell, quoteString(str).data());
    } else if (cell->isObject()) {
        if (!cell->inherits(&JSC::JSNotAnObject::s_info)) {
            if (cell->inherits(&JSC::JSFunction::s_info)) {
                dumpFunction(exec, userData, cb, const_cast<JSC::JSFunction*>(static_cast<const JSC::JSFunction*>(cell)), indent);
            } else {
                dumpObject(exec, userData, cb, cell->getObject(), indent);
            }
        }
    } else if (cell->isGetterSetter()) {
        cb(userData, "{ \"type\": \"Getter/setter\" }\n");
    } else if (cell->isAPIValueWrapper()) {
        cb(userData, "{ \"type\": \"API value wrapper\" }\n");
    } else {
        cb(userData, "{ \"type\": \"Unknown cell\", \"id\": \"%p\", \"typeInfo\": %d }\n",
           cell, cell->structure() ? cell->structure()->typeInfo().type() : -1);
    }
    indent -= 4;
    cb(userData, "%*s}\n", indent, "");
}

class Netflix_StringImplCreate
{
public:
    static WTF::StringImpl* create(const JSChar* chars, size_t size)
    {
        return new WTF::StringImpl(chars, size);
    }
};

extern "C" {

JS_EXPORT bool Netflix_JSValueIsArray(JSContextRef ctx, JSValueRef value)
{
    JSC::ExecState *exec = toJS(ctx);
    JSC::APIEntryShim shim(exec);
    JSC::JSValue val = toJS(exec, value);
    if (val.isCell()) {
        JSC::JSCell *cell = val.asCell();
        const JSC::ClassInfo *classInfo = cell->classInfo();
        while (classInfo) {
            if (!strcmp(classInfo->className, "Array")) {
                return true;
                break;
            }
            classInfo = classInfo->parentClass;
        }
    }
    return false;
}

JS_EXPORT void Netflix_DumpJSHeap(JSContextRef ctx, void *userData, DumpCallback cb)
{
    JSC::ExecState *exec = toJS(ctx);
    JSC::APIEntryShim shim(exec);
    JSC::JSGlobalData &globalData = exec->globalData();
    JSC::JSGlobalObject *globalObject = exec->lexicalGlobalObject();
    if (!globalObject) {
        return;
    }
    NetflixDebugger* debugger = NetflixDebugger::get(globalObject);
    if (debugger->attachRef) // temporarily detach the debugger
        debugger->detach(globalObject);

    NetflixForEachBlock foreach;
    globalData.heap.objectSpace().forEachBlock<NetflixForEachBlock>(foreach);

    size_t blockSizes = 0, blockCapacities = 0;
    WTF::Vector<NetflixForEachCell::Block>::const_iterator blockit = foreach.foreachCell.blocks.begin();
    const WTF::Vector<NetflixForEachCell::Block>::const_iterator blockend = foreach.foreachCell.blocks.end();
    cb(userData, "{ \"data\": [\n");
    while (blockit != blockend) {
        cb(userData, "{ \"type\": \"Block\", \"cellSize\": %u, \"blockSize\": %u, \"blockCapacity\": %u, \"cells\": [\n",
           blockit->cellSize, blockit->blockSize, blockit->blockCapacity);
        blockSizes += blockit->blockSize;
        blockCapacities += blockit->blockCapacity;
        NetflixForEachCell::currentCellSize = blockit->cellSize;
        WTF::Vector<JSC::JSCell*>::const_iterator cellit = blockit->cells.begin();
        const WTF::Vector<JSC::JSCell*>::const_iterator cellend = blockit->cells.end();
        while (cellit != cellend) {
            dumpCell(exec, userData, cb, *cellit, 4);
            ++cellit;
            if (cellit != cellend)
                cb(userData, "%*s,\n", 4, "");
        }
        cb(userData, "\n]}\n");
        ++blockit;
        if (blockit != blockend)
            cb(userData, ",\n");
    }
    cb(userData, "], \"totalSize\": %u, \"totalCapacity\": %u }\n", globalData.heap.size(), globalData.heap.capacity());
    if (debugger->attachRef) // reattach the debugger if needed
        debugger->attach(globalObject);
}

JS_EXPORT void Netflix_DumpJSHeapCounts(JSContextRef ctx, void *userData, DumpCallback cb)
{
    JSC::ExecState *exec = toJS(ctx);
    JSC::APIEntryShim shim(exec);
    JSC::JSGlobalData &globalData = exec->globalData();
    JSC::JSGlobalObject *globalObject = exec->lexicalGlobalObject();
    if (!globalObject) {
        return;
    }
    NetflixDebugger* debugger = NetflixDebugger::get(globalObject);
    if (debugger->attachRef) // temporarily detach the debugger
        debugger->detach(globalObject);

    NetflixForEachBlock foreach;
    globalData.heap.objectSpace().forEachBlock<NetflixForEachBlock>(foreach);

    WTF::Vector<NetflixForEachCell::Block>::const_iterator blockit = foreach.foreachCell.blocks.begin();
    const WTF::Vector<NetflixForEachCell::Block>::const_iterator blockend = foreach.foreachCell.blocks.end();

    char buffer[132];
    while (blockit != blockend) {
        NetflixForEachCell::currentCellSize = blockit->cellSize;
        WTF::Vector<JSC::JSCell*>::const_iterator cellit = blockit->cells.begin();
        const WTF::Vector<JSC::JSCell*>::const_iterator cellend = blockit->cells.end();
        while (cellit != cellend) {
            JSC::JSCell * cell = *cellit;
            unsigned int length = 0;
            if (cell->isString()) {
                JSC::UString str = cell->getString(exec);
                length = str.length();
                if (!str.is8Bit())
                    length *= 2;
                CString u = quoteString(str);
                buffer[131] = 0;
                buffer[130] = '.';
                buffer[129] = '.';
                buffer[128] = '.';
                buffer[0] = 0;
                if (u.data()) {
                    strncpy(buffer, u.data(), 128);
                    for(char * p = buffer; *p; ++p) {
                        if (*p < ' ')
                            *p = '.';
                    }
                }
                cb(userData, "%s,%lu,%u,\"%s\"\n", "string", blockit->cellSize, length, buffer);
            }
            else {
                const char * klass = "unknown";
                if (const JSC::ClassInfo * ci = cell->classInfo()) {
                    if (const char * name = ci->className) {
                        klass = name;
                    }
                }
                cb(userData, "%s,%lu\n", klass, blockit->cellSize);
            }
            ++cellit;
        }
        ++blockit;
    }
    if (debugger->attachRef) // reattach the debugger if needed
        debugger->attach(globalObject);
}


JS_EXPORT void Netflix_ResetDebugger(JSContextRef ctx)
{
    JSC::ExecState *exec = toJS(ctx);
    JSC::APIEntryShim shim(exec);
    JSC::JSGlobalObject *global = exec->lexicalGlobalObject();
    if (!global)
        return;
    NetflixDebugger::reset(global);
}

JS_EXPORT int Netflix_SaveDebuggerState(JSContextRef ctx)
{
    JSC::ExecState *exec = toJS(ctx);
    JSC::APIEntryShim shim(exec);
    JSC::JSGlobalObject *global = exec->lexicalGlobalObject();
    if (!global)
        return -1;
    NetflixDebugger* debugger = NetflixDebugger::get(global);
    return debugger->saveBreakpoints();
}

JS_EXPORT void Netflix_RestoreDebuggerState(JSContextRef ctx, int id)
{
    JSC::ExecState *exec = toJS(ctx);
    JSC::APIEntryShim shim(exec);
    JSC::JSGlobalObject *global = exec->lexicalGlobalObject();
    if (!global)
        return;
    NetflixDebugger* debugger = NetflixDebugger::get(global);
    debugger->restoreBreakpoints(id);
    if (!debugger->breaks.isEmpty()) {
        if (!debugger->attachRef) {
            debugger->attach(global);
            debugger->recompileAllJSFunctions(&global->globalData());
        }
        ++debugger->attachRef;
    }
}

JS_EXPORT void Netflix_SetBreakpointCallback(JSContextRef ctx, BreakpointCallback callback, void* userData)
{
    JSC::ExecState *exec = toJS(ctx);
    JSC::APIEntryShim shim(exec);
    JSC::JSGlobalObject *global = exec->lexicalGlobalObject();
    if (!global)
        return;
    NetflixDebugger* debugger = NetflixDebugger::get(global);
    debugger->callback = callback;
    debugger->userData = userData;
}

JS_EXPORT void Netflix_ResolveSourceID(intptr_t sourceID, JSStringRef* file)
{
    JSC::SourceProvider* provider = idToSourceProvider(sourceID);
    const JSC::UString url = provider->url();
    *file = JSStringCreateWithCharacters(url.characters(), url.length());
}

JS_EXPORT bool Netflix_EvaluateAtBreakpoint(JSContextRef ctx, JSStringRef code, JSValueRef* ret, JSValueRef* exception, int frameno)
{
    JSC::UString str(JSStringGetCharactersPtr(code), JSStringGetLength(code));
    JSC::ExecState *exec = toJS(ctx);
    JSC::APIEntryShim shim(exec);
    JSC::JSGlobalObject *global = exec->lexicalGlobalObject();
    if (!global) {
        if (ret)
            *ret = JSValueMakeUndefined(ctx);
        if (exception)
            *exception = JSValueMakeUndefined(ctx);
        return false;
    }
    NetflixDebugger* debugger = NetflixDebugger::get(global);
    if (debugger->callstack.isEmpty()) {
        if (ret)
            *ret = JSValueMakeUndefined(ctx);
        if (exception)
            *exception = JSValueMakeUndefined(ctx);
        return false;
    }
    JSC::JSValue e;
    const JSC::DebuggerCallFrame& frame = !frameno ? debugger->callstack.currentFrame() : debugger->callstack.frameAt(frameno);
    const JSC::JSValue r = frame.evaluate(str, e);
    if (ret) {
        if (r.isEmpty() || !e.isEmpty())
            *ret = toRef(exec, JSC::jsUndefined());
        else
            *ret = toRef(exec, r);
    }
    if (exception) {
        *exception = !e.isEmpty() ? toRef(exec, e) : JSValueMakeUndefined(ctx);
    }
    return true;
}

JS_EXPORT void Netflix_Pause(JSContextRef ctx)
{
    JSC::ExecState *exec = toJS(ctx);
    JSC::APIEntryShim shim(exec);
    JSC::JSGlobalObject *global = exec->lexicalGlobalObject();
    if (!global)
        return;
    NetflixDebugger* debugger = NetflixDebugger::get(global);
    debugger->pauseOnNextStatement();
}

JS_EXPORT void Netflix_AttachDebugger(JSContextRef ctx)
{
    JSC::ExecState *exec = toJS(ctx);
    JSC::APIEntryShim shim(exec);
    JSC::JSGlobalObject *global = exec->lexicalGlobalObject();
    if (!global)
        return;
    NetflixDebugger* debugger = NetflixDebugger::get(global);
    if (!debugger->attachRef) {
        debugger->attach(global);
        debugger->recompileAllJSFunctions(&global->globalData());
    }
    ++debugger->attachRef;
}

JS_EXPORT void Netflix_DetachDebugger(JSContextRef ctx)
{
    JSC::ExecState *exec = toJS(ctx);
    JSC::APIEntryShim shim(exec);
    JSC::JSGlobalObject *global = exec->lexicalGlobalObject();
    if (!global)
        return;
    NetflixDebugger* debugger = NetflixDebugger::get(global);
    if (!--debugger->attachRef) {
        debugger->detach(global);
        debugger->callstack.clear();
        debugger->recompileAllJSFunctions(&global->globalData());
    }
}

static inline void setBreakpoint(JSContextRef ctx, NetflixBreakpoint& point, unsigned int* position)
{
    JSC::ExecState *exec = toJS(ctx);
    JSC::APIEntryShim shim(exec);
    JSC::JSGlobalObject *global = exec->lexicalGlobalObject();
    if (!global) {
        *position = static_cast<unsigned int>(-1);
        return;
    }
    NetflixDebugger* debugger = NetflixDebugger::get(global);
    if (!debugger->attachRef) {
        assert(debugger->breaks.isEmpty());
        debugger->attach(global);
        if (!global->globalData().dynamicGlobalObject)
            debugger->recompileAllJSFunctions(&global->globalData());
        debugger->resolve(&point);
        ++debugger->attachRef;
    } else {
        if (debugger->callstack.isEmpty())
            ++debugger->attachRef;
        debugger->resolve(&point);
        const size_t pos = debugger->breaks.find(point);
        if (pos != WTF::notFound)
            debugger->breaks.remove(pos);
    }
    debugger->breaks.append(point);
    *position = debugger->breaks.size() - 1;
}

JS_EXPORT void Netflix_SetBreakpoint(JSContextRef ctx, JSStringRef file, int line, unsigned int* position)
{
    JSC::UString f(JSStringGetCharactersPtr(file), JSStringGetLength(file));
    NetflixBreakpoint point(f, line);
    setBreakpoint(ctx, point, position);
}

JS_EXPORT void Netflix_SetConditionalBreakpoint(JSContextRef ctx, unsigned int id, JSStringRef condition)
{
    JSC::ExecState *exec = toJS(ctx);
    JSC::APIEntryShim shim(exec);
    JSC::JSGlobalObject *global = exec->lexicalGlobalObject();
    if (!global)
        return;
    NetflixDebugger* debugger = NetflixDebugger::get(global);
    if (debugger->breaks.size() <= id)
        return;
    JSC::UString c(JSStringGetCharactersPtr(condition), JSStringGetLength(condition));
    NetflixBreakpoint& point = debugger->breaks[id];
    point.condition = c;
}

JS_EXPORT void Netflix_RemoveBreakpoint(JSContextRef ctx, unsigned int id)
{
    JSC::ExecState *exec = toJS(ctx);
    JSC::APIEntryShim shim(exec);
    JSC::JSGlobalObject *global = exec->lexicalGlobalObject();
    if (!global)
        return;
    NetflixDebugger* debugger = NetflixDebugger::get(global);
    if (id == 0 || id > debugger->breaks.size())
        return;
    debugger->breaks.remove(id - 1);
    if (debugger->breaks.isEmpty() && !--debugger->attachRef) {
        debugger->detach(global);
        debugger->callstack.clear();
        debugger->recompileAllJSFunctions(&global->globalData());
    }
}

// duplicated in ScriptEngineJSC.cpp
struct Netflix_JSBreakpoint
{
    JSStringRef url, condition;
    int line;
};

JS_EXPORT int Netflix_ListBreakpoints(JSContextRef ctx, Netflix_JSBreakpoint *breakpoints, int max)
{
    JSC::ExecState *exec = toJS(ctx);
    JSC::APIEntryShim shim(exec);
    JSC::JSGlobalObject *global = exec->lexicalGlobalObject();
    if (!global) {
        return 0;
    }
    NetflixDebugger* debugger = NetflixDebugger::get(global);
    if (debugger->breaks.isEmpty()) {
        return 0;
    }

    WTF::String str;
    WTF::Vector<NetflixBreakpoint>::const_iterator it = debugger->breaks.begin();
    const WTF::Vector<NetflixBreakpoint>::const_iterator end = debugger->breaks.end();
    int i = 0;
    while (it != end && i < max) {
        breakpoints[i].line = it->line;
        breakpoints[i].url = JSStringCreateWithCharacters(it->file.characters(), it->file.length());
        breakpoints[i].condition = JSStringCreateWithCharacters(it->condition.characters(), it->condition.length());
        ++it;
        ++i;
    }
    return i;
}

JS_EXPORT void Netflix_StepOver(JSContextRef ctx)
{
    JSC::ExecState *exec = toJS(ctx);
    JSC::APIEntryShim shim(exec);
    JSC::JSGlobalObject *global = exec->lexicalGlobalObject();
    if (!global)
        return;
    NetflixDebugger* debugger = NetflixDebugger::get(global);
    if (debugger->callstack.isEmpty())
        return;
    debugger->callstack.setStepState(NetflixStack::StepOver);
}

JS_EXPORT void Netflix_StepInto(JSContextRef ctx)
{
    JSC::ExecState *exec = toJS(ctx);
    JSC::APIEntryShim shim(exec);
    JSC::JSGlobalObject *global = exec->lexicalGlobalObject();
    if (!global)
        return;
    NetflixDebugger* debugger = NetflixDebugger::get(global);
    if (debugger->callstack.isEmpty())
        return;
    debugger->callstack.setStepState(NetflixStack::StepInto);
}

JS_EXPORT void Netflix_Continue(JSContextRef ctx)
{
    JSC::ExecState *exec = toJS(ctx);
    JSC::APIEntryShim shim(exec);
    JSC::JSGlobalObject *global = exec->lexicalGlobalObject();
    if (!global)
        return;
    NetflixDebugger* debugger = NetflixDebugger::get(global);
    if (debugger->callstack.isEmpty())
        return;
    debugger->callstack.clearStepStates();
}

JS_EXPORT void Netflix_Finish(JSContextRef ctx)
{
    JSC::ExecState *exec = toJS(ctx);
    JSC::APIEntryShim shim(exec);
    JSC::JSGlobalObject *global = exec->lexicalGlobalObject();
    if (!global)
        return;
    NetflixDebugger* debugger = NetflixDebugger::get(global);
    if (debugger->callstack.size() < 2)
        return;
    debugger->callstack.setStepState(NetflixStack::StepNone);
    debugger->callstack.setCallerStepState(NetflixStack::StepOver);
}

// duplicated in ScriptEngineJSC.cpp
struct Netflix_JSStackFrame
{
    JSStringRef url, function;
    int line, column;
};

JS_EXPORT int Netflix_StackTrace(JSContextRef ctx, Netflix_JSStackFrame *frames, int max)
{
    JSC::ExecState *exec = toJS(ctx);
    JSC::APIEntryShim shim(exec);
    JSC::JSGlobalObject *global = exec->lexicalGlobalObject();
    if (!global)
        return 0;
    NetflixDebugger* debugger = NetflixDebugger::get(global);
    size_t stackSize = debugger->callstack.size();
    if (static_cast<size_t>(max) < stackSize)
        stackSize = max;
    for (size_t i = 1; i <= stackSize; ++i) {
        const JSC::DebuggerCallFrame& frame = debugger->callstack.frameAt(i);
        const intptr_t sourceID = debugger->callstack.sourceIDAt(i);
        frames[i - 1].line = debugger->callstack.lineNumberAt(i);
        frames[i - 1].column = debugger->callstack.columnNumberAt(i);
        JSC::SourceProvider* provider = idToSourceProvider(sourceID);
        const JSC::UString fname = frame.calculatedFunctionName();
        frames[i - 1].function = JSStringCreateWithCharacters(fname.characters(), fname.length());
        const JSC::UString url = provider->url();
        frames[i - 1].url = JSStringCreateWithCharacters(url.characters(), url.length());
    }
    return stackSize;
}

JS_EXPORT JSValueRef Netflix_Interpreter_StackTrace(JSContextRef ctx, JSObjectRef objectRef)
{
    JSC::CallFrame *callFrame = toJS(ctx);
    JSC::JSGlobalData* globalData = &callFrame->globalData();
    JSC::APIEntryShim shim(globalData);
    Vector<JSC::StackFrame> stackTrace;
    JSC::Interpreter::getStackTrace(globalData, stackTrace);

    JSC::JSObject *object = toJS(objectRef);
    JSC::JSGlobalObject *globalObject = object->globalObject();
    StringBuilder builder;
    for (unsigned i = 0; i < stackTrace.size(); i++) {
        builder.append(String(stackTrace[i].toString(globalObject->globalExec()).impl()));
        if (i != stackTrace.size() - 1)
            builder.append('\n');
    }

    return toRef(callFrame, jsString(globalData, JSC::UString(builder.toString().impl())));
}

JS_EXPORT void Netflix_SetCurrentFrame(JSContextRef ctx, size_t frameNo)
{
    JSC::ExecState *exec = toJS(ctx);
    JSC::APIEntryShim shim(exec);
    JSC::JSGlobalObject *global = exec->lexicalGlobalObject();
    if (!global)
        return;
    NetflixDebugger* debugger = NetflixDebugger::get(global);
    if (debugger->callstack.size() < 2)
        return;
    debugger->callstack.setFrameNumber(frameNo);
}

JS_EXPORT size_t Netflix_GetCurrentFrame(JSContextRef ctx)
{
    JSC::ExecState *exec = toJS(ctx);
    JSC::APIEntryShim shim(exec);
    JSC::JSGlobalObject *global = exec->lexicalGlobalObject();
    if (!global)
        return 0;
    NetflixDebugger* debugger = NetflixDebugger::get(global);
    if (debugger->callstack.size() < 2)
        return 0;
    return debugger->callstack.frameNumber();
}

JS_EXPORT bool Netflix_Identifiers(JSContextRef ctx, JSStringRef** identifiers, size_t* count, int mode, int frameno)
{
    enum { Global = -1 };

    const bool isGlobal = (mode == Global);

    JSC::ExecState *exec = toJS(ctx);
    JSC::APIEntryShim shim(exec);
    JSC::JSGlobalObject *global = exec->lexicalGlobalObject();
    if (!global) {
        *count = 0;
        return false;
    }
    NetflixDebugger* debugger = NetflixDebugger::get(global);
    if (debugger->callstack.isEmpty()) {
        *count = 0;
        return false;
    }

    int activationCount = -1;

    WTF::Vector<JSC::UString> idents;
    const JSC::DebuggerCallFrame& frame = (!frameno) ? debugger->callstack.currentFrame() : debugger->callstack.frameAt(frameno);
    JSC::ScopeChainIterator scopeit = frame.scopeChain()->begin();
    const JSC::ScopeChainIterator scopeend = frame.scopeChain()->end();
    while (scopeit != scopeend) {
        JSC::JSObject* o = scopeit->get();
        if (o->isActivationObject()) { // first activation object is local, subsequent are closures
            ++activationCount;
            if (isGlobal || activationCount < mode) {
                ++scopeit;
                continue;
            }
        } else {
            if (!isGlobal) {
                ++scopeit;
                continue;
            }
        }
        JSC::PropertyNameArray names(global->globalExec());
        o->methodTable()->getPropertyNames(o, global->globalExec(), names, JSC::ExcludeDontEnumProperties);
        JSC::PropertyNameArray::const_iterator nameit = names.begin();
        const JSC::PropertyNameArray::const_iterator nameend = names.end();
        while (nameit != nameend) {
            idents.append(nameit->ustring());
            ++nameit;
        }
        if (!isGlobal)
            break;
        ++scopeit;
    }
    if (idents.isEmpty()) {
        *count = 0;
        return isGlobal;
    }

    *identifiers = static_cast<JSStringRef*>(malloc(sizeof(JSStringRef) * idents.size()));
    JSStringRef* cur = *identifiers;
    WTF::Vector<JSC::UString>::const_iterator identit = idents.begin();
    const WTF::Vector<JSC::UString>::const_iterator identend = idents.end();
    while (identit != identend) {
        *cur = JSStringCreateWithCharacters(identit->characters(), identit->length());
        ++cur;
        ++identit;
    }
    *count = idents.size();
    return true;
}

JS_EXPORT size_t Netflix_GarbageCollect(JSContextRef ctx)
{
    if (NetflixGlobalObject::useBackdoorGarbageCollect()) {
        JSC::ExecState *exec = toJS(ctx);
        JSC::APIEntryShim shim(exec);
        JSC::JSGlobalObject *global = exec->lexicalGlobalObject();
        if (!global)
            return 0;
        JSC::JSGlobalData& data = global->globalData();
        const size_t prev = data.heap.size();
        data.heap.collectAllGarbage();
        const size_t now = data.heap.size();
        return (prev > now) ? prev - now : 0;
    } else {
        JSGarbageCollect(ctx);
        return 0;
    }
}

JS_EXPORT bool Netflix_HeapInfo(JSContextRef ctx, size_t *size, size_t *capacity,
                                size_t *objectCount, size_t *globalObjectCount,
                                size_t *protectedObjectCount,
                                size_t *protectedGlobalObjectCount)
{
    JSC::ExecState *exec = toJS(ctx);
    JSC::APIEntryShim shim(exec);
    JSC::JSGlobalObject *global = exec->lexicalGlobalObject();
    if (!global) {
        if (size)
            *size = 0;
        if (capacity)
            *capacity = 0;
        if (objectCount)
            *objectCount = 0;
        if (globalObjectCount)
            *globalObjectCount = 0;
        if (protectedObjectCount)
            *protectedObjectCount = 0;
        if (protectedGlobalObjectCount)
            *protectedGlobalObjectCount = 0;
        return false;
    }

    JSC::JSGlobalData& data = global->globalData();
    if (size)
        *size = data.heap.size();
    if (capacity)
        *capacity = data.heap.capacity();
    if (objectCount)
        *objectCount = data.heap.objectCount();
    if (globalObjectCount)
        *globalObjectCount = data.heap.globalObjectCount();
    if (protectedObjectCount)
        *protectedObjectCount = data.heap.protectedObjectCount();
    if (protectedGlobalObjectCount)
        *protectedGlobalObjectCount = data.heap.protectedGlobalObjectCount();

    return true;
}

JS_EXPORT void Netflix_JSFunctionCode(JSContextRef ctx, JSObjectRef object, JSStringRef* str)
{
    JSC::ExecState *exec = toJS(ctx);
    JSC::APIEntryShim shim(exec);

    *str = 0;
    const JSC::JSObject* obj = toJS(object);
    if (!obj->inherits(&JSC::JSNotAnObject::s_info)) {
        if (obj->inherits(&JSC::JSFunction::s_info)) {
            const JSC::JSFunction* func = static_cast<const JSC::JSFunction*>(obj);
            const JSC::SourceCode* code = func->sourceCode();
            if (code) {
                const JSC::UString source = code->toString();
                *str = JSStringCreateWithCharacters(source.characters(), source.length());
            }
        }
    }
}

JS_EXPORT int Netflix_IgnoreExceptions(JSContextRef ctx, int ignore)
{
    JSC::ExecState *exec = toJS(ctx);
    JSC::APIEntryShim shim(exec);
    JSC::JSGlobalObject *global = exec->lexicalGlobalObject();
    if (!global)
        return -1;
    NetflixDebugger* debugger = NetflixDebugger::get(global);
    const NetflixDebugger::ExceptionBreak was = debugger->breakOnException;
    if (ignore < 0 || ignore > 3)
        return -1;
    debugger->breakOnException = static_cast<NetflixDebugger::ExceptionBreak>(ignore);
    return was;
}

JS_EXPORT void Netflix_JSCInitializeThreading()
{
    JSC::initializeThreading();
}

JS_EXPORT void Netflix_WTFInitializeMainThread()
{
    WTF::initializeMainThread();
}

JS_EXPORT JSStringRef Netflix_JSStringCreate(const char* string, int length)
{
    JSC::initializeThreading();
    if (string) {
        WTF::Vector<UChar, 1024> buffer(length);
        UChar* p = buffer.data();
        const char *copy = string;
        if (WTF::Unicode::convertUTF8ToUTF16(&copy, string + length, &p, p + length) == WTF::Unicode::conversionOK) {
            return OpaqueJSString::create(buffer.data(), p - buffer.data()).leakRef();
        }

        p = buffer.data();
        for (int i = 0; i < length; ++i)
            p[i] = string[i];
        JSStringRef ref = JSStringCreateWithCharacters(p, length);
        return ref;
    }

    // Null string.
    return OpaqueJSString::create().leakRef();
}

JS_EXPORT JSStringRef Netflix_JSStringAppend(JSStringRef left, const char *string, int length)
{
    if (!string || !length)
        return left;

    const int currentLength = JSStringGetLength(left);
    if (!currentLength)
        return Netflix_JSStringCreate(string, length);

    WTF::Vector<UChar, 1024> buffer(length + currentLength);
    memcpy(buffer.data(), JSStringGetCharactersPtr(left), sizeof(UChar) * currentLength);
    UChar* p = buffer.data() + currentLength;
    const char *copy = string;
    if (WTF::Unicode::convertUTF8ToUTF16(&copy, string + length, &p, p + length) == WTF::Unicode::conversionOK) {
        return OpaqueJSString::create(buffer.data(), p - buffer.data()).leakRef();
    }

    p = buffer.data() + currentLength;
    for (int i = 0; i < length; ++i)
        p[i] = string[i];
    JSStringRef ref = JSStringCreateWithCharacters(p, length);
    return ref;
}

JS_EXPORT JSContextGroupRef Netflix_JSContextGroupCreate()
{
    JSC::initializeThreading();
    return toRef(JSC::JSGlobalData::createContextGroup(JSC::ThreadStackTypeLarge).leakRef());
}

JS_EXPORT JSGlobalContextRef Netflix_JSGlobalContextCreateInGroup(JSContextGroupRef group)
{
    assert(group);

    JSC::initializeThreading();

    JSC::JSLock lock(JSC::LockForReal);
    WTF::RefPtr<JSC::JSGlobalData> globalData = WTF::PassRefPtr<JSC::JSGlobalData>(toJS(group));

    JSC::APIEntryShim entryShim(globalData.get(), false);

    globalData->makeUsableFromMultipleThreads();

    NetflixGlobalObject* globalObject = NetflixGlobalObject::create(*globalData, NetflixGlobalObject::createStructure(*globalData, JSC::jsNull()));
    return JSGlobalContextRetain(toGlobalRef(globalObject->globalExec()));
}

JS_EXPORT void Netflix_ReleaseExecutableMemory(JSContextRef ctx)
{
    JSC::ExecState *exec = toJS(ctx);
    JSC::APIEntryShim shim(exec);
    JSC::JSGlobalObject *global = exec->lexicalGlobalObject();
    if (!global)
        return;
    global->globalData().releaseExecutableMemory();
}

JS_EXPORT void Netflix_SetJSTimeoutInterval(JSContextRef ctx, unsigned int interval, TimeoutCallback callback, void *userData)
{
    JSC::ExecState *exec = toJS(ctx);
    JSC::APIEntryShim shim(exec);
    JSC::JSGlobalObject *global = exec->lexicalGlobalObject();
    if (!global)
        return;
    global->globalData().timeoutChecker.setTimeoutInterval(interval);
    NetflixGlobalObject::setTimeoutCallback(interval, callback, userData);
}

JS_EXPORT void Netflix_UseBackdoorGarbageCollect(bool use)
{
    NetflixGlobalObject::setUseBackdoorGarbageCollect(use);
}

JS_EXPORT void Netflix_Global_Stacktrace(JSContextRef ctx, void *userData, DumpCallback cb)
{
    JSC::ExecState *exec = toJS(ctx);
    JSC::APIEntryShim shim(exec);
    JSC::JSGlobalObject *global = exec->lexicalGlobalObject();
    if (!global)
        return;

    JSC::JSGlobalData& globalData = global->globalData();
    Vector<JSC::StackFrame> stackTrace;
    JSC::Interpreter::getStackTrace(&globalData, stackTrace);

    JSC::CallFrame* frame = globalData.topCallFrame;

    for (unsigned i = 0; i < stackTrace.size(); i++) {
        cb(userData, "%u: %s\n", i, stackTrace[i].toString(frame).utf8().data());
    }
}

JS_EXPORT const JSChar* Netflix_JSValueGetString(JSContextRef ctx, JSValueRef value, size_t* size)
{
    JSC::ExecState* exec = toJS(ctx);
    JSC::APIEntryShim entryShim(exec);

    JSC::JSValue jsValue = toJS(exec, value);
    const JSC::UString& str = jsValue.toString(exec)->value(exec);
    if (size)
        *size = str.length();
    return str.characters();
}

JS_EXPORT JSValueRef Netflix_JSStringCreateValue(JSContextRef ctx, const JSChar* chars, size_t length)
{
    JSC::ExecState* exec = toJS(ctx);
    JSC::APIEntryShim entryShim(exec);

    WTF::StringImpl* impl = Netflix_StringImplCreate::create(chars, length);
    const JSC::UString ustr(impl);
    impl->deref();
    return toRef(exec, jsString(exec, ustr));
}

JS_EXPORT JSChar* Netflix_CreateJSChar(size_t length)
{
    return static_cast<JSChar*>(fastMalloc(length * sizeof(JSChar)));
}

JS_EXPORT size_t Netflix_JSValueGetUTF8CString(const JSChar* chars, size_t size, char* buffer, size_t bufferSize)
{
    if (!bufferSize)
        return 0;

    char* p = buffer;
    const UChar* b = chars;
    WTF::Unicode::ConversionResult result = WTF::Unicode::convertUTF16ToUTF8(&b, chars + size, &p, p + bufferSize - 1, true);
    *p++ = '\0';
    if (result != WTF::Unicode::conversionOK && result != WTF::Unicode::targetExhausted)
        return 0;

    return p - buffer;
}

struct Netflix_JSPropertyNameArray
{
public:
    Netflix_JSPropertyNameArray(JSC::JSGlobalData* global)
        : refcnt(1), array(global)
    {
    }

    void ref() { ++refcnt; }
    void deref() { if (!--refcnt) delete this; }

    int refcnt;
    JSC::PropertyNameArray array;
};

typedef void* Netflix_JSPropertyNameArrayRef;
JS_EXPORT  Netflix_JSPropertyNameArrayRef Netflix_JSObjectPropertyNames(JSContextRef ctx, JSObjectRef object)
{
    JSC::JSObject* jsObject = toJS(object);
    JSC::ExecState* exec = toJS(ctx);
    JSC::APIEntryShim entryShim(exec);

    JSC::JSGlobalData* globalData = &exec->globalData();

    Netflix_JSPropertyNameArray* array = new Netflix_JSPropertyNameArray(globalData);
    jsObject->methodTable()->getPropertyNames(jsObject, exec, array->array, JSC::ExcludeDontEnumProperties);
    return array;
}

Netflix_JSPropertyNameArrayRef Netflix_JSPropertyNameArrayRetain(Netflix_JSPropertyNameArrayRef array)
{
    Netflix_JSPropertyNameArray* a = static_cast<Netflix_JSPropertyNameArray*>(array);
    a->ref();
    return array;
}

JS_EXPORT void Netflix_JSPropertyNameArrayRelease(JSContextRef ctx, Netflix_JSPropertyNameArrayRef array)
{
    JSC::ExecState* exec = toJS(ctx);
    JSC::APIEntryShim entryShim(exec);

    Netflix_JSPropertyNameArray* a = static_cast<Netflix_JSPropertyNameArray*>(array);
    a->deref();
}

JS_EXPORT size_t Netflix_JSPropertyNameArrayGetCount(Netflix_JSPropertyNameArrayRef array)
{
    Netflix_JSPropertyNameArray* a = static_cast<Netflix_JSPropertyNameArray*>(array);
    return a->array.size();
}

JS_EXPORT const JSChar* Netflix_JSPropertyNameArrayGetNameAtIndex(JSContextRef ctx, Netflix_JSPropertyNameArrayRef array, size_t index, size_t* size)
{
    JSC::ExecState* exec = toJS(ctx);
    JSC::APIEntryShim entryShim(exec);

    Netflix_JSPropertyNameArray* a = static_cast<Netflix_JSPropertyNameArray*>(array);
    const JSC::Identifier& ident = a->array[index];
    if (size)
        *size = ident.ustring().length();
    return ident.ustring().characters();
}

JS_EXPORT JSValueRef Netflix_JSObjectGetProperty(JSContextRef ctx, JSObjectRef object, Netflix_JSPropertyNameArrayRef array, size_t index, JSValueRef* exception)
{
    JSC::ExecState* exec = toJS(ctx);
    JSC::APIEntryShim entryShim(exec);

    JSC::JSObject* jsObject = toJS(object);

    Netflix_JSPropertyNameArray* a = static_cast<Netflix_JSPropertyNameArray*>(array);
    JSC::JSValue jsValue = jsObject->get(exec, a->array[index]);
    if (exec->hadException()) {
        if (exception)
            *exception = toRef(exec, exec->exception());
        exec->clearException();
    }
    return toRef(exec, jsValue);
}

JS_EXPORT void Netflix_StartCPUProfiler(JSContextRef ctx, int profileId)
{
    if (NetflixGlobalObject::supportsProfiling(0))
        return;
    Netflix_AttachDebugger(ctx);

    JSC::ExecState* exec = toJS(ctx);
    JSC::APIEntryShim entryShim(exec);

    NetflixGlobalObject::setProfilerEnabled(true);
    // make sure we recompile JS
    JSC::JSGlobalObject *global = exec->lexicalGlobalObject();
    if (!global)
        return;
    NetflixDebugger* debugger = NetflixDebugger::get(global);
    debugger->recompileAllJSFunctions(&global->globalData());

    char buf[32];
    snprintf(buf, sizeof(buf), "CPU%d", profileId);
    JSC::Profiler* profiler = JSC::Profiler::profiler();
    profiler->startProfiling(exec, buf);
}

static void serializeCPUNode(JSC::ProfileNode* node, void* userData, DumpCallback cb, double total)
{
    //printf("node %s: selftime %f (%f) totaltime %f (%f)\n", node->functionName().utf8().data(), node->selfTime(), node->actualSelfTime(), node->totalTime(), node->actualTotalTime());
    int hitCount = node->selfTime();

    cb(userData, "{ \"functionName\": \"%s\", \"scriptId\": \"%s\", \"url\": \"%s\", \"lineNumber\": %d, \"columnNumber\": %d, \"hitCount\": %d, \"callUID\": %d, \"deoptReason\": \"%s\", \"children\": [", node->functionName().utf8().data(), node->url().utf8().data(), node->url().utf8().data(), node->lineNumber(), 1, hitCount, node->callIdentifier().hash(), "");
    const WTF::Vector<WTF::RefPtr<JSC::ProfileNode> >& children = node->children();
    const size_t csize = children.size();
    for (size_t i = 0; i < csize; ++i) {
        serializeCPUNode(children[i].get(), userData, cb, total);
        if (i + 1 < csize)
            cb(userData, ",");
    }
    cb(userData, "]}\n");
}

JS_EXPORT void Netflix_StopCPUProfiler(JSContextRef ctx, int profileId, void *userData, DumpCallback cb)
{
    if (!NetflixGlobalObject::supportsProfiling(0))
        return;
    {
        JSC::ExecState* exec = toJS(ctx);
        JSC::APIEntryShim entryShim(exec);

        char buf[32];
        snprintf(buf, sizeof(buf), "CPU%d", profileId);
        JSC::Profiler* profiler = JSC::Profiler::profiler();
        if (!profiler)
            return;
        WTF::RefPtr<JSC::Profile> profile = profiler->stopProfiling(exec, buf);
        NetflixGlobalObject::setProfilerEnabled(false);

        JSC::ProfileNode* head = profile->head();
        profile->focus(head);
        cb(userData, "{ \"startTime\": %f, \"endTime\": %f, \"self\": %f, \"head\": ", head->startTime(), head->startTime() + head->totalTime(), head->selfTime());
        serializeCPUNode(head, userData, cb, head->totalTime());
        cb(userData, "}\n");

        // recompile
        JSC::JSGlobalObject *global = exec->lexicalGlobalObject();
        if (!global)
            return;
        NetflixDebugger* debugger = NetflixDebugger::get(global);
        debugger->recompileAllJSFunctions(&global->globalData());
    }
    Netflix_DetachDebugger(ctx);
}

JS_EXPORT void Netflix_SetGCMaxBytesPerCycle(size_t size)
{
    JSC::Heap::setMaxBytesPerCycle(size);
}

JS_EXPORT void Netflix_SetFastMallocCallback(MallocCallback callback)
{
    WTF::setFastMallocCallback(callback);
}

JS_EXPORT void Netflix_QueryMalloc(uint64_t* system, uint64_t* metadata, uint64_t* freebytes, size_t* returned)
{
    WTF::queryFastMalloc(system, metadata, freebytes, returned);
}

JS_EXPORT void Netflix_TerminateScavengerThread()
{
    WTF::terminateJSCScavengeThread();
}

#ifdef NRDP_JSC_ENABLE_OSALLOCATOR_CALLBACKS
typedef void (*OSAllocatorCallback)(void *, size_t);
typedef void (*OSAllocatorStartCallback)();
typedef void(*StringImplRefCallback)(void *, const char *, const int16_t *, unsigned);
typedef void(*StringImplDerefCallback)(void *);
JS_EXPORT void Netflix_SetAllocationCallbacks(OSAllocatorCallback allocate, OSAllocatorCallback release,
                                              OSAllocatorCallback commit, OSAllocatorCallback decommit,
                                              OSAllocatorStartCallback start,
                                              StringImplRefCallback stringImplRefCallback,
                                              StringImplDerefCallback stringImplDerefCallback)
{
    OSAllocator::setCallbacks(allocate, release, commit, decommit, start);
    StringImpl::setCallbacks(stringImplRefCallback, stringImplDerefCallback);
}
#endif

#ifdef NRDP_JSC_ENABLE_GC_CALLBACKS
JS_EXPORT void Netflix_SetGCCallbacks(JSC::Heap::GCPhaseCallback callback)
{
    JSC::Heap::setCallback(callback);
}
#endif

} // extern "C"

void NetflixGlobalObject::dumpStackTrace(const JSC::JSGlobalObject* obj)
{
    if (!timeoutCallback)
        return;

    JSC::JSGlobalData& globalData = obj->globalData();
    Vector<JSC::StackFrame> stackTrace;
    JSC::Interpreter::getStackTrace(&globalData, stackTrace);

    JSC::CallFrame* frame = globalData.topCallFrame;

    char buf[1024];
    snprintf(buf, sizeof(buf), "JS code has been running for a long time (%ums)", timeoutInterval);
    timeoutCallback(timeoutUserData, buf);
    for (unsigned i = 0; i < stackTrace.size(); i++) {
        snprintf(buf, sizeof(buf), "%u/%u: %s", i, stackTrace.size(),
                 stackTrace[i].toString(frame).utf8().data());
        timeoutCallback(timeoutUserData, buf);
    }
}
