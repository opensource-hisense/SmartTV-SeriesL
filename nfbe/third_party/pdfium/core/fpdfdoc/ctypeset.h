// Copyright 2016 PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#ifndef CORE_FPDFDOC_CTYPESET_H_
#define CORE_FPDFDOC_CTYPESET_H_

#include "core/fpdfdoc/cpvt_floatrect.h"
#include "core/fxcrt/include/fx_system.h"

class CPDF_VariableText;
class CSection;

class CTypeset {
 public:
  explicit CTypeset(CSection* pSection);
  virtual ~CTypeset();
  CPVT_Size GetEditSize(FX_FLOAT fFontSize);
  CPVT_FloatRect Typeset();
  CPVT_FloatRect CharArray();

 private:
  void SplitLines(FX_BOOL bTypeset, FX_FLOAT fFontSize);
  void OutputLines();

  CPVT_FloatRect m_rcRet;
  CPDF_VariableText* m_pVT;
  CSection* const m_pSection;
};

#endif  // CORE_FPDFDOC_CTYPESET_H_
