#include "content/renderer/oipf_plugin/oipf_avcomponent.h"

namespace content {

OipfAvComponent::OipfAvComponent(int componentTag, int pid, int type,
    const std::string& encoding, bool encrypted)
    : component_tag_(componentTag),
      pid_(pid),
      type_(type),
      encoding_(encoding),
      encrypted_(encrypted) {
}

OipfAvComponent::~OipfAvComponent() {
}

bool OipfAvComponent::isAudio() {
  return false;
}

bool OipfAvComponent::isVideo() {
  return false;
}

bool OipfAvComponent::isText() {
  return false;
}

OipfAvAudioComponent* OipfAvComponent::toAudio() {
 return nullptr;
}

OipfAvVideoComponent* OipfAvComponent::toVideo() {
  return nullptr;
}

OipfAvTextComponent* OipfAvComponent::toText() {
  return nullptr;
}

}  // namespace content
