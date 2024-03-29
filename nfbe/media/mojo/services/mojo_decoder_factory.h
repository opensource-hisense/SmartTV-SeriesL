// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MEDIA_MOJO_SERVICES_MOJO_DECODER_FACTORY_H_
#define MEDIA_MOJO_SERVICES_MOJO_DECODER_FACTORY_H_

#include "base/macros.h"
#include "media/base/decoder_factory.h"
#include "mojo/shell/public/interfaces/interface_provider.mojom.h"

namespace media {

class MojoDecoderFactory : public DecoderFactory {
 public:
  explicit MojoDecoderFactory(
      mojo::shell::mojom::InterfaceProvider* interface_provider);
  ~MojoDecoderFactory() final;

  void CreateAudioDecoders(
      scoped_refptr<base::SingleThreadTaskRunner> task_runner,
      ScopedVector<AudioDecoder>* audio_decoders) final;

  void CreateVideoDecoders(
      scoped_refptr<base::SingleThreadTaskRunner> task_runner,
      ScopedVector<VideoDecoder>* video_decoders) final;

 private:
  mojo::shell::mojom::InterfaceProvider* interface_provider_;

  DISALLOW_COPY_AND_ASSIGN(MojoDecoderFactory);
};

}  // namespace media

#endif  // MEDIA_MOJO_SERVICES_MOJO_DECODER_FACTORY_H_
