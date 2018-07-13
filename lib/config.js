/*
 * Copyright (C) 2018 Daniel Anderson
 *
 * This source code is licensed under the MIT license found in the LICENSE file
 * in the root directory of this source tree.
 */

'use strict';

const is = require('./is');
const os = require('os');
const native = require('bindings')('pixels-please');

function setThreadPoolSize(size) {
    if (!is.int(size) || size < 0) {
        throw Error('Invalid thread pool size.');
    }

    if (size === 0) {
        size = os.cpus().length;
    }

    native.setThreadPoolSize(size);
}

module.exports = (Pixels) =>  {
    /**
     * Gets or sets the internal image processing thread pool size. By default, the pool size is equal to the
     * number of cpu cores on the system. Setting threads to 0 will reset the pool size to the default.
     *
     * @static
     * @name Pipeline.threads
     * @throws {Error} when setting a value other than a positive integer
     */
    Object.defineProperty(Pixels, "threads", {
            get: native.getThreadPoolSize,
            set: setThreadPoolSize,
            enumerable: true
        }
    );
};
