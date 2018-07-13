/*
 * Copyright (C) 2018 Daniel Anderson
 *
 * This source code is licensed under the MIT license found in the LICENSE file
 * in the root directory of this source tree.
 */

'use strict';

const assert = require('chai').assert;
const Pipeline = require('../lib');

const FILENAME = '/path/image.png';

describe("constructor module test", () => {
    describe("Pipeline()", () => {
        it("should construct with function", () => {
            assert.instanceOf(Pipeline(FILENAME), Pipeline);
        });
        it("should construct with new", () => {
            assert.instanceOf(new Pipeline(FILENAME), Pipeline);
        });
        it("should throw Error with no arg", () => {
            assert.throws(() => Pipeline());
        });
        it("should throw Error with invalid arg", () => {
            [null, '', 4].forEach(arg => assert.throws(() => Pipeline(arg)));
        });
    });
});