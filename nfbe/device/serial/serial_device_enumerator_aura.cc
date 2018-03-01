// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "device/serial/serial_device_enumerator_aura.h"

#include <stdint.h>
#include <utility>

#include "base/logging.h"

namespace device {

// static
scoped_ptr<SerialDeviceEnumerator> SerialDeviceEnumerator::Create() {
  return scoped_ptr<SerialDeviceEnumerator>(new SerialDeviceEnumeratorAura());
}

SerialDeviceEnumeratorAura::SerialDeviceEnumeratorAura() {
}

SerialDeviceEnumeratorAura::~SerialDeviceEnumeratorAura() {}

mojo::Array<serial::DeviceInfoPtr> SerialDeviceEnumeratorAura::GetDevices() {
  mojo::Array<serial::DeviceInfoPtr> devices((size_t)0);
  return devices;
}

}  // namespace device
