// Copyright 2016 ACCESS CO. LTD. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef WebXMLAIT_h
#define WebXMLAIT_h

#include "WebCommon.h"
#include "WebString.h"

namespace blink {

// This class is used in the FrameLoaderImpl to pass XML AIT parameters
// from the Document to the RenderFrameImpl
class BLINK_PLATFORM_EXPORT WebXMLAIT {
public:
    unsigned short m_aitAppID;
    unsigned int m_aitOrgID;
    WebString m_aitControlCode;
    WebString m_aitVisibility;
    bool m_aitServiceBound;
    unsigned char m_aitPriority;
    WebString m_aitUrlBase;
    WebString m_aitApplicationLocation;
};

}

#endif // WebXMLAIT_h
