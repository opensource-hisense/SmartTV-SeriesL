/*
 *  Copyright (C) 2006 George Staikos <staikos@kde.org>
 *  Copyright (C) 2006 Alexey Proskuryakov <ap@nypop.com>
 *  Copyright (C) 2006, 2007, 2008, 2009 Apple Inc. All rights reserved.
 *  Copyright (C) 2014 Netflix, Inc. All rights reserved.
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Library General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Library General Public License for more details.
 *
 *  You should have received a copy of the GNU Library General Public License
 *  along with this library; see the file COPYING.LIB.  If not, write to
 *  the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 *  Boston, MA 02110-1301, USA.
 *
 */

#ifndef WTF_UNICODE_GIBBON_H
#define WTF_UNICODE_GIBBON_H

#include <stdlib.h>
#include <string.h>
#include <algorithm>
#include <wtf/ASCIICType.h>
#include <wtf/unicode/UnicodeMacrosFromICU.h>

#if !defined(WIN32) && !defined(_WIN32) \
    && !((defined(__CC_ARM) || defined(__ARMCC__)) && !defined(__linux__)) /* RVCT */
    typedef unsigned short UChar;
#else
    typedef wchar_t UChar;
#endif

typedef int32_t UChar32;

namespace WTF {
namespace Unicode {

enum Direction {
    LeftToRight,
    RightToLeft,
    EuropeanNumber,
    EuropeanNumberSeparator,
    EuropeanNumberTerminator,
    ArabicNumber,
    CommonNumberSeparator,
    BlockSeparator,
    SegmentSeparator,
    WhiteSpaceNeutral,
    OtherNeutral,
    LeftToRightEmbedding,
    LeftToRightOverride,
    RightToLeftArabic,
    RightToLeftEmbedding,
    RightToLeftOverride,
    PopDirectionalFormat,
    NonSpacingMark,
    BoundaryNeutral
};

enum DecompositionType {
    DecompositionNone,
    DecompositionCanonical,
    DecompositionCompat,
    DecompositionCircle,
    DecompositionFinal,
    DecompositionFont,
    DecompositionFraction,
    DecompositionInitial,
    DecompositionIsolated,
    DecompositionMedial,
    DecompositionNarrow,
    DecompositionNoBreak,
    DecompositionSmall,
    DecompositionSquare,
    DecompositionSub,
    DecompositionSuper,
    DecompositionVertical,
    DecompositionWide
};

enum CharCategory {
    NoCategory,
    Other_NotAssigned,
    Letter_Uppercase,
    Letter_Lowercase,
    Letter_Titlecase,
    Letter_Modifier,
    Letter_Other,

    Mark_NonSpacing,
    Mark_Enclosing,
    Mark_SpacingCombining,

    Number_DecimalDigit,
    Number_Letter,
    Number_Other,

    Separator_Space,
    Separator_Line,
    Separator_Paragraph,

    Other_Control,
    Other_Format,
    Other_PrivateUse,
    Other_Surrogate,

    Punctuation_Dash,
    Punctuation_Open,
    Punctuation_Close,
    Punctuation_Connector,
    Punctuation_Other,

    Symbol_Math,
    Symbol_Currency,
    Symbol_Modifier,
    Symbol_Other,

    Punctuation_InitialQuote,
    Punctuation_FinalQuote
};

inline unsigned char gibbonUtf16ToUChar(UChar32 c)
{
    if (c > 0xff)
        return 0;
    return static_cast<unsigned char>(c);
}

inline char gibbonUtf16ToChar(UChar32 c)
{
    if (c > 0x7f)
        return 0;
    return static_cast<char>(c);
}

typedef UChar32 (*GibbonUtf16Func)(UChar32);
inline int gibbonUtf16Transform(const UChar* src, int srcLength, UChar* dst, int dstLength, GibbonUtf16Func func)
{
    const int len = std::min(srcLength, dstLength);
    for (int i = 0; i < len; ++i) {
        *(dst++) = func(*(src++));
    }
    return len;
}

inline UChar32 foldCase(UChar32 c)
{
    if (c < 0xff && c >= 0xe0) {
        return c - 0x20;
    }
    return toASCIIUpper(gibbonUtf16ToUChar(c));
}

inline int foldCase(UChar* result, int resultLength, const UChar* src, int srcLength, bool* error)
{
    const int realLength = gibbonUtf16Transform(src, srcLength, result, resultLength, foldCase);
    *error = false;
    return realLength;
}

inline UChar32 toLower(UChar32 c)
{
    if (c < 0xdf && c >= 0xc0) {
        return c + 0x20;
    }
    return toASCIILower(gibbonUtf16ToUChar(c));
}

inline UChar32 toUpper(UChar32 c)
{
    if (c < 0xff && c >= 0xe0) {
        return c - 0x20;
    }
    return toASCIIUpper(gibbonUtf16ToUChar(c));
}

inline int toLower(UChar* result, int resultLength, const UChar* src, int srcLength, bool* error)
{
    const int realLength = gibbonUtf16Transform(src, srcLength, result, resultLength, toLower);
    *error = false;
    return realLength;
}

inline int toUpper(UChar* result, int resultLength, const UChar* src, int srcLength, bool* error)
{
    const int realLength = gibbonUtf16Transform(src, srcLength, result, resultLength, toUpper);
    *error = false;
    return realLength;
}

/*
inline UChar32 toTitleCase(UChar32 c)
{
    return u_totitle(c);
}

inline bool isArabicChar(UChar32 c)
{
      return ublock_getCode(c) == UBLOCK_ARABIC;
}

inline bool isAlphanumeric(UChar32 c)
{
    return u_isalnum(c);
}
*/

inline bool isSeparatorSpace(UChar32 c)
{
    return isASCIISpace(gibbonUtf16ToUChar(c));
}

/*
inline bool isPrintableChar(UChar32 c)
{
    return !!u_isprint(c);
}

inline bool isPunct(UChar32 c)
{
    return !!u_ispunct(c);
}

inline bool hasLineBreakingPropertyComplexContext(UChar32 c)
{
    return u_getIntPropertyValue(c, UCHAR_LINE_BREAK) == U_LB_COMPLEX_CONTEXT;
}

inline bool hasLineBreakingPropertyComplexContextOrIdeographic(UChar32 c)
{
    int32_t prop = u_getIntPropertyValue(c, UCHAR_LINE_BREAK);
    return prop == U_LB_COMPLEX_CONTEXT || prop == U_LB_IDEOGRAPHIC;
}

inline UChar32 mirroredChar(UChar32 c)
{
    return u_charMirror(c);
}
*/

inline CharCategory category(UChar32 c)
{
    const char ac = gibbonUtf16ToChar(c);
    if (isASCIILower(ac))
        return Letter_Lowercase;
    if (isASCIIUpper(ac))
        return Letter_Uppercase;
    if (isASCIIDigit(ac))
        return Number_DecimalDigit;
    if (isASCIISpace(ac))
        return Separator_Space;
    if (ac == '\n' || ac == '\r' || ac == '\f')
        return Separator_Line;
    return Other_NotAssigned;
}

inline Direction direction(UChar32 /*c*/)
{
    return LeftToRight;
}

inline bool isLower(UChar32 c)
{
    return isASCIILower(gibbonUtf16ToUChar(c));
}

/*
inline uint8_t combiningClass(UChar32 c)
{
    return u_getCombiningClass(c);
}

inline DecompositionType decompositionType(UChar32 c)
{
    return static_cast<DecompositionType>(u_getIntPropertyValue(c, UCHAR_DECOMPOSITION_TYPE));
}
*/

inline int umemcasecmp(const UChar* a, const UChar* b, int len)
{
    char ca, cb;
    int res;
    for (int i = 0; i < len; ++i) {
        ca = gibbonUtf16ToChar(a[i]);
        cb = gibbonUtf16ToChar(b[i]);
        res = strncmp(&ca, &cb, 1);
        if (res)
            return res;
    }
    return 0;
}

} }

#endif // WTF_UNICODE_GIBBON_H
