/*
 * Copyright (C) 2018 Daniel Anderson
 *
 * This source code is licensed under the MIT license found in the LICENSE file
 * in the root directory of this source tree.
 */

'use strict';

const assert = require('chai').assert;
const Pipeline = require('../lib');

describe("config module test", () => {
    describe("threads property", () => {
        it("should be greater than zero", () => {
            assert.isAbove(Pipeline.threads, 0);
        });
        it("should reset size when assigned to 0", () => {
            Pipeline.threads = 1;
            assert.equal(Pipeline.threads, 1);
            Pipeline.threads = 0;
            assert.isAbove(Pipeline.threads, 0);
        });
        it("should throw Error when assigned a negative size", () => {
            assert.throws(() => Pipeline.threads = -1);
        });
        it("should throw Error when assigned something other than number", () => {
            assert.throws(() => Pipeline.threads = 'invalid');
        });
    });
});
