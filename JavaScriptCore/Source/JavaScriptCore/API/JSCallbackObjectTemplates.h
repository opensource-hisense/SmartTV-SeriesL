
#include "Heap.h"
#include <wtf/text/StringHash.h>

namespace JSC {

ASSERT_CLASS_FITS_IN_CELL(JSCallbackObject<JSNonFinalObject>);
ASSERT_CLASS_FITS_IN_CELL(JSCallbackObject<JSGlobalObject>);

// Define the two types of JSCallbackObjects we support.
template <> const ClassInfo JSCallbackObject<JSNonFinalObject>::s_info = { "CallbackObject", &JSNonFinalObject::s_info, 0, 0, CREATE_METHOD_TABLE(JSCallbackObject) };
template <> const ClassInfo JSCallbackObject<JSGlobalObject>::s_info = { "CallbackGlobalObject", &JSGlobalObject::s_info, 0, 0, CREATE_METHOD_TABLE(JSCallbackObject) };

template <>
Structure* JSCallbackObject<JSNonFinalObject>::createStructure(JSGlobalData& globalData, JSGlobalObject* globalObject, JSValue proto)
{
    return Structure::create(globalData, globalObject, proto, TypeInfo(ObjectType, StructureFlags), &s_info);
}

template <>
Structure* JSCallbackObject<JSGlobalObject>::createStructure(JSGlobalData& globalData, JSGlobalObject* globalObject, JSValue proto)
{
    return Structure::create(globalData, globalObject, proto, TypeInfo(GlobalObjectType, StructureFlags), &s_info);
}

template <class Parent>
void JSCallbackObject<Parent>::destroy(JSCell* cell)
{
    jsCast<JSCallbackObject*>(cell)->JSCallbackObject::~JSCallbackObject();
}

} // namespace JSC
