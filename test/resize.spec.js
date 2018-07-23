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

const TEST_IMAGE = 'test/resources/one.png';
const TEST_SVG = 'test/resources/rounded-rect.svg';
const TEST_TALL = 'test/resources/tall.png';
const TEST_WIDE = 'test/resources/wide.png';

describe("resize module test", () => {
    describe("resize()", () => {
        it("should use the default of 'fit' when resizing", () => {
            [TEST_IMAGE, TEST_SVG].forEach(source => {
                const buffer = Pipeline(source)
                    .bytes()
                    .resize(20, 20)
                    .toBufferSync();
                assert.equal(buffer.header.width, 20);
                assert.equal(buffer.header.height, 20);
            })
        });
    });
    describe("contain()", () => {
        it("should not resize if image is fully inside resize bounding box", () => {
            testContainInside(false);
        });
        it("should resize a 20x200 to 10x100 with a 100x100 bounding box", () => {
            const buffer = Pipeline(TEST_TALL)
                .bytes()
                .contain()
                .resize(100, 100)
                .toBufferSync();
            assert.equal(buffer.header.width, 10);
            assert.equal(buffer.header.height, 100);
        });
        it("should resize a 200x20 to 100x10 with a 100x100 bounding box", () => {
            const buffer = Pipeline(TEST_WIDE)
                .bytes()
                .contain()
                .resize(100, 100)
                .toBufferSync();
            assert.equal(buffer.header.width, 100);
            assert.equal(buffer.header.height, 10);
        });
        it("should resize a 100x100 to 50x50 with a 50x50 bounding box", () => {
            const buffer = Pipeline(TEST_SVG)
                .bytes()
                .contain()
                .resize(50, 50)
                .toBufferSync();
            assert.equal(buffer.header.width, 50);
            assert.equal(buffer.header.height, 50);
        });
    });
    describe("fit()", () => {
        it("should resize to fit the resize bounding box", () => {
            const buffer = Pipeline(TEST_IMAGE)
                .bytes()
                .fit()
                .resize(20, 20)
                .toBufferSync();
            assert.equal(buffer.header.width, 20);
            assert.equal(buffer.header.height, 20);
        });
        it("should resize a 20x200 to 10x100 with a 100x100 bounding box", () => {
            const buffer = Pipeline(TEST_TALL)
                .bytes()
                .fit()
                .resize(100, 100)
                .toBufferSync();
            assert.equal(buffer.header.width, 10);
            assert.equal(buffer.header.height, 100);
        });
        it("should resize a 200x20 to 100x10 with a 100x100 bounding box", () => {
            const buffer = Pipeline(TEST_WIDE)
                .bytes()
                .fit()
                .resize(100, 100)
                .toBufferSync();
            assert.equal(buffer.header.width, 100);
            assert.equal(buffer.header.height, 10);
        });
        it("should resize a 100x100 to 50x50 with a 50x50 bounding box", () => {
            const buffer = Pipeline(TEST_SVG)
                .bytes()
                .fit()
                .resize(50, 50)
                .toBufferSync();
            assert.equal(buffer.header.width, 50);
            assert.equal(buffer.header.height, 50);
        });
    });
    describe("ignoreAspectRatio()", () => {
        it("should not resize if image is fully inside resize bounding box", () => {
            testContainInside(true);
        });
        it("should resize a 20x200 to 20x100 with a 100x100 bounding box", () => {
            const buffer = Pipeline(TEST_TALL)
                .bytes()
                .contain()
                .ignoreAspectRatio()
                .resize(100, 100)
                .toBufferSync();
            assert.equal(buffer.header.width, 10);
            assert.equal(buffer.header.height, 100);
        });
        it("should resize a 200x20 to 100x20 with a 100x100 bounding box", () => {
            const buffer = Pipeline(TEST_WIDE)
                .bytes()
                .contain()
                .ignoreAspectRatio()
                .resize(100, 100)
                .toBufferSync();
            assert.equal(buffer.header.width, 100);
            assert.equal(buffer.header.height, 10);
        });
        it("should resize a 20x200 to 20x100 with a 100x100 bounding box", () => {
            const buffer = Pipeline(TEST_TALL)
                .bytes()
                .fit()
                .ignoreAspectRatio()
                .resize(100, 100)
                .toBufferSync();
            assert.equal(buffer.header.width, 10);
            assert.equal(buffer.header.height, 100);
        });
        it("should resize a 200x20 to 100x20 with a 100x100 bounding box", () => {
            const buffer = Pipeline(TEST_WIDE)
                .bytes()
                .fit()
                .ignoreAspectRatio()
                .resize(100, 100)
                .toBufferSync();
            assert.equal(buffer.header.width, 100);
            assert.equal(buffer.header.height, 10);
        });
    });
    describe("filter()", () => {
        it("should resize with all available filters", () => {
            ['box', 'tent', 'gaussian'].forEach(filter => {
                const buffer = Pipeline(TEST_IMAGE)
                    .bytes()
                    .filter(filter)
                    .resize(2, 2)
                    .toBufferSync();
                assert.equal(buffer.header.width, 2);
                assert.equal(buffer.header.height, 2);
            });
        });
        it("should resize SVG", () => {
            const buffer = Pipeline(TEST_SVG)
                .bytes()
                .filter('gaussian')
                .resize(300, 300)
                .toBufferSync();
            assert.equal(buffer.header.width, 300);
            assert.equal(buffer.header.height, 300);
        });
        it("should resize SVG with filter", () => {
            const buffer = Pipeline(TEST_SVG)
                .bytes()
                .filter('gaussian', {disableDecoderScaling: true})
                .resize(300, 300)
                .toBufferSync();
            assert.equal(buffer.header.width, 300);
            assert.equal(buffer.header.height, 300);
        });
        it("should throw when filter arg is invalid", () => {
            [null, '', 'not a filter'].forEach(input => {
                assert.throws(() => Pipeline(TEST_IMAGE).filter(input));
            })
        });
    });
});

function testContainInside(ignoreAspectRatio) {
    [TEST_IMAGE, TEST_SVG, TEST_TALL, TEST_WIDE].forEach(source => {
        const header = Pipeline(source)
            .bytes()
            .toHeaderSync();

        const pipeline = Pipeline(source)
            .bytes()
            .contain()
            .resize(500, 500);

        if (ignoreAspectRatio) {
            pipeline.ignoreAspectRatio();
        }

        const buffer = pipeline.toBufferSync();

        assert.equal(buffer.header.width, header.width);
        assert.equal(buffer.header.height, header.height);
    });
}