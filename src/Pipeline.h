/*
 * Copyright (C) 2018 Daniel Anderson
 *
 * This source code is licensed under the MIT license found in the LICENSE file
 * in the root directory of this source tree.
 */
 
#ifndef PIPELINE_H
#define PIPELINE_H

#include <napi.h>

void LoadPipeline(const Napi::CallbackInfo& info);
Napi::Value LoadPipelineSync(const Napi::CallbackInfo& info);

#endif
