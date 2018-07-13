/*
 * Copyright (C) 2018 Daniel Anderson
 *
 * This source code is licensed under the MIT license found in the LICENSE file
 * in the root directory of this source tree.
 */
 
#include "Threads.h"

using namespace Napi;

int GetInitialThreadPoolSize();

ctpl::thread_pool sThreadPool(GetInitialThreadPoolSize());

ctpl::thread_pool& GetThreadPool() {
    return sThreadPool;
}

int GetInitialThreadPoolSize() {
    auto count = (int)std::thread::hardware_concurrency();
    return count == 0 ? 4 : count;
}

Value GetThreadPoolSize(const CallbackInfo& info) {
    return Number::New(info.Env(), sThreadPool.size());
}

void SetThreadPoolSize(const CallbackInfo& info) {
    auto size = info[0].As<Number>().Int32Value();
    sThreadPool.resize(size);
}
