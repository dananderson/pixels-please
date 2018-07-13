/*
 * Copyright (C) 2018 Daniel Anderson
 *
 * This source code is licensed under the MIT license found in the LICENSE file
 * in the root directory of this source tree.
 */
 
#include <napi.h>
#include "Threads.h"
#include "Pipeline.h"

using namespace Napi;

Object Init(Env env, Object exports) {
    exports["loadPipeline"] = Function::New(env, LoadPipeline, "loadPipeline");
    exports["loadPipelineSync"] = Function::New(env, LoadPipelineSync, "loadPipelineSync");
    exports["setThreadPoolSize"] = Function::New(env, SetThreadPoolSize, "setThreadPoolSize");
    exports["getThreadPoolSize"] = Function::New(env, GetThreadPoolSize, "getThreadPoolSize");

    return exports;
}

NODE_API_MODULE(pixels, Init);
