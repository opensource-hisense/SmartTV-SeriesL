// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef DEVICE_SERIAL_SERIAL_DEVICE_ENUMERATOR_AURA_H_
#define DEVICE_SERIAL_SERIAL_DEVICE_ENUMERATOR_AURA_H_

#include "base/macros.h"
#include "device/serial/serial_device_enumerator.h"

namespace device {

// Discovers and enumerates serial devices available to the host.
// This is only a stub class for Aura to be able to build with use_udev=0
// TODO: Implement proper enumeration without UDEV
class SerialDeviceEnumeratorAura : public SerialDeviceEnumerator {
 public:
  SerialDeviceEnumeratorAura();
  ~SerialDeviceEnumeratorAura() override;

  // Implementation for SerialDeviceEnumerator.
  mojo::Array<serial::DeviceInfoPtr> GetDevices() override;

 private:

  DISALLOW_COPY_AND_ASSIGN(SerialDeviceEnumeratorAura);
};

}  // namespace device

#endif  // DEVICE_SERIAL_SERIAL_DEVICE_ENUMERATOR_AURA_H_
