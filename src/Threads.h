/*
 * Copyright (C) 2018 Daniel Anderson
 *
 * This source code is licensed under the MIT license found in the LICENSE file
 * in the root directory of this source tree.
 */
 
#ifndef THREADS_H
#define THREADS_H

#include <napi.h>
#include "cptl_stl.h"

ctpl::thread_pool& GetThreadPool();
Napi::Value GetThreadPoolSize(const Napi::CallbackInfo& info);
void SetThreadPoolSize(const Napi::CallbackInfo& info);

#endif
