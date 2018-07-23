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

const FILE_NOT_FOUND_FILENAME = 'doesnotexist.jpg';
const TEST_RESOURCES_DIR = 'test/resources';
const TEST_IMAGES = [ 'one.bmp', 'one.gif', 'one.jpg', 'one.png', 'one.psd', 'one.tga', 'one.hdr', 'one.ppm', 'one.pgm' ];
const TEST_SVG = TEST_RESOURCES_DIR + '/rounded-rect.svg';

describe('output module test', () => {
    describe('toBuffer()', () => {
        it('should load all supported image formats', () => {
            return assert.isFulfilled(
                Promise.all(
                    TEST_IMAGES.map(image => {
                        return Pipeline(`${TEST_RESOURCES_DIR}/${image}`)
                            .bytes()
                            .toBuffer()
                            .then(checkBuffer);
                    })
                )
            );
        });
        it('should reject when file not found', () => {
            return assert.isRejected(Pipeline(FILE_NOT_FOUND_FILENAME)
                .bytes()
                .toBuffer());
        });
        it('should load SVG', () => {
            return assert.isFulfilled(Pipeline(TEST_SVG)
                .bytes()
                .toBuffer()
                .then((buffer) => checkSvgBuffer(buffer)));
        });
        it('should fail when SVG has no width and height', () => {
            return assert.isRejected(Pipeline(`${TEST_RESOURCES_DIR}/bad.svg`)
                .bytes()
                .toBuffer());
        });
    });
    describe('toBufferSync()', () => {
        it('should load all supported image formats', () => {
            TEST_IMAGES.map(image => Pipeline(`${TEST_RESOURCES_DIR}/${image}`).bytes().toBufferSync())
                .forEach(checkBuffer);
        });
        it('should throw Error when file not found', () => {
            assert.throws(() => Pipeline(FILE_NOT_FOUND_FILENAME).bytes().toBufferSync());
        });
        it('should load SVG', () => {
            checkSvgBuffer(Pipeline(TEST_SVG).bytes().toBufferSync());
        });
    });
    describe('toHeader()', () => {
        it('should reject when file not found', () => {
            return assert.isRejected(Pipeline(FILE_NOT_FOUND_FILENAME)
                .bytes()
                .toHeader());
        });
        it('should load all supported image formats', () => {
            return assert.isFulfilled(
                Promise.all(
                    TEST_IMAGES.map(image => {
                        return Pipeline(`${TEST_RESOURCES_DIR}/${image}`)
                            .toHeader()
                            .then(checkHeader);
                    })
                )
            );
        });
        it('should load SVG', () => {
            return Pipeline(TEST_SVG)
                .toHeader()
                .then(checkSvgHeader);
        });
    });
    describe('toHeaderSync()', () => {
        it('should throw Error when file not found', () => {
            assert.throws(() => Pipeline(FILE_NOT_FOUND_FILENAME).bytes().toHeaderSync());
        });
        it('should load all supported image formats', () => {
            TEST_IMAGES.map(image => Pipeline(`${TEST_RESOURCES_DIR}/${image}`).toHeaderSync())
                .forEach(checkHeader);
        });
        it('should load SVG', () => {
            checkSvgHeader(Pipeline(TEST_SVG).toHeaderSync());
        });
    });
});

function checkHeader(header) {
    assert.equal(header.width, 1);
    assert.equal(header.height, 1);
    assert.equal(header.channels, 4);
}

function checkSvgHeader(header) {
    assert.equal(header.width, 100);
    assert.equal(header.height, 100);
    assert.equal(header.channels, 4);
}

function checkBufferHeader(header) {
    checkHeader(header);
    assert.isString(header.format);
}

function checkSvgBufferHeader(header) {
    checkSvgHeader(header);
    assert.isString(header.format);
}

function checkSvgBuffer(buffer) {
    assert.exists(buffer.header);
    checkSvgBufferHeader(buffer.header);
    assert.equal(buffer.length, buffer.header.channels*buffer.header.width*buffer.header.height);
}

function checkBuffer(buffer) {
    assert.exists(buffer.header);
    checkBufferHeader(buffer.header);
    assert.equal(buffer.length, buffer.header.channels);    
}
