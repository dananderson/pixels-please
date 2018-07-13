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
    });
    describe('toBufferSync()', () => {
        it('should load all supported image formats', () => {
            TEST_IMAGES.map(image => Pipeline(`${TEST_RESOURCES_DIR}/${image}`).bytes().toBufferSync())
                .forEach(checkBuffer);
        });
        it('should throw Error when file not found', () => {
            assert.throws(() => Pipeline(FILE_NOT_FOUND_FILENAME).bytes().toBufferSync());
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
    });
    describe('toHeaderSync()', () => {
        it('should throw Error when file not found', () => {
            assert.throws(() => Pipeline(FILE_NOT_FOUND_FILENAME).bytes().toHeaderSync());
        });
        it('should load all supported image formats', () => {
            TEST_IMAGES.map(image => Pipeline(`${TEST_RESOURCES_DIR}/${image}`).toHeaderSync())
                .forEach(checkHeader);
        });
    });
});

function checkHeader(header) {
    assert.equal(header.width, 1);
    assert.equal(header.height, 1);
    assert.equal(header.channels, 4);
}

function checkBufferHeader(header) {
    checkHeader(header);
    assert.isString(header.format);
}

function checkBuffer(buffer) {
    assert.exists(buffer.header);
    checkBufferHeader(buffer.header);
    assert.equal(buffer.length, buffer.header.channels);    
}
