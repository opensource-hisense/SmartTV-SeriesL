// Copyright 2014 PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#ifndef XFA_FXBARCODE_COMMON_BC_COMMONECI_H_
#define XFA_FXBARCODE_COMMON_BC_COMMONECI_H_

#include <stdint.h>

class CBC_CommonECI {
 public:
  CBC_CommonECI(int32_t value);
  virtual ~CBC_CommonECI();

  int32_t GetValue();
  static CBC_CommonECI* GetEICByValue(int32_t value, int32_t& e);

 private:
  int32_t m_value;
};

#endif  // XFA_FXBARCODE_COMMON_BC_COMMONECI_H_
