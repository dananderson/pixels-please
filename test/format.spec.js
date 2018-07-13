/*
 * Copyright (C) 2018 Daniel Anderson
 *
 * This source code is licensed under the MIT license found in the LICENSE file
 * in the root directory of this source tree.
 */

'use strict';

const chai = require('chai');
chai.use(require('chai-as-promised'));
const assert = chai.assert;
const Pipeline = require('../lib');

const THREE_CHANNEL_IMAGE = 'test/resources/one.bmp';
const FOUR_CHANNEL_IMAGE = 'test/resources/one.png';

describe("format module test", () => {
    describe("bytes()", () => {
        it("should accept all valid pixel format options", () => {
            ['rgba', 'argb', 'abgr', 'bgra', 'keep'].forEach(format => Pipeline(FOUR_CHANNEL_IMAGE).bytes({format}));
        });
        it("should throw Error for invalid pixel format", () => {
            [null, '', 4, 'rgbx'].forEach(format => assert.throws(() => Pipeline(FOUR_CHANNEL_IMAGE).bytes({format})));
        });
        describe("with four channel source image", () => {
            it("should produce rgba pixels", () => {
                pixelFormatTest(FOUR_CHANNEL_IMAGE, 'rgba', 0x0B151FFF, 0xFF1F150B);
            });
            it("should produce argb pixels", () => {
                pixelFormatTest(FOUR_CHANNEL_IMAGE, 'argb', 0xFF0B151F, 0x1F150BFF);
            });
            it("should produce abgr pixels", () => {
                pixelFormatTest(FOUR_CHANNEL_IMAGE, 'abgr', 0xFF1F150B, 0x0B151FFF);
            });
            it("should produce bgra pixels", () => {
                pixelFormatTest(FOUR_CHANNEL_IMAGE, 'bgra', 0x1F150BFF, 0xFF0B151F);
            });
        });
        describe("with three channel source image", () => {
            it("should produce rgba pixels", () => {
                pixelFormatTest(THREE_CHANNEL_IMAGE, 'rgba', 0x0B151FFF, 0xFF1F150B);
            });
            it("should produce argb pixels", () => {
                pixelFormatTest(THREE_CHANNEL_IMAGE, 'argb', 0xFF0B151F, 0x1F150BFF);
            });
            it("should produce abgr pixels", () => {
                pixelFormatTest(THREE_CHANNEL_IMAGE, 'abgr', 0xFF1F150B, 0x0B151FFF);
            });
            it("should produce bgra pixels", () => {
                pixelFormatTest(THREE_CHANNEL_IMAGE, 'bgra', 0x1F150BFF, 0xFF0B151F);
            });
        });
    });
});

function pixelFormatTest(filename, format, pixelLE, pixelBE) {
    const buffer = Pipeline(filename).bytes({format}).toBufferSync();

    assert.equal(pixelLE, buffer.readUInt32LE(0));
    assert.equal(pixelBE, buffer.readUInt32BE(0));
}