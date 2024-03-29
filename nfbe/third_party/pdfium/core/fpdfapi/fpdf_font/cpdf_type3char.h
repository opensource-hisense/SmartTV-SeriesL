// Copyright 2016 PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#ifndef CORE_FPDFAPI_FPDF_FONT_CPDF_TYPE3CHAR_H_
#define CORE_FPDFAPI_FPDF_FONT_CPDF_TYPE3CHAR_H_

#include "core/fxcrt/include/fx_coordinates.h"
#include "core/fxcrt/include/fx_system.h"

class CFX_DIBitmap;
class CPDF_Form;
class CPDF_RenderContext;

class CPDF_Type3Char {
 public:
  // Takes ownership of |pForm|.
  explicit CPDF_Type3Char(CPDF_Form* pForm);
  ~CPDF_Type3Char();

  FX_BOOL LoadBitmap(CPDF_RenderContext* pContext);

  CPDF_Form* m_pForm;
  CFX_DIBitmap* m_pBitmap;
  FX_BOOL m_bColored;
  int m_Width;
  CFX_Matrix m_ImageMatrix;
  FX_RECT m_BBox;
};

#endif  // CORE_FPDFAPI_FPDF_FONT_CPDF_TYPE3CHAR_H_
