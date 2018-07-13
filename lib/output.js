/*
 * Copyright (C) 2018 Daniel Anderson
 *
 * This source code is licensed under the MIT license found in the LICENSE file
 * in the root directory of this source tree.
 */

'use strict';

const native = require('bindings')('pixels');

/**
 * Image information.
 *
 * @typedef {Object} Header
 * @property {int} width The width of the image.
 * @property {int} height The height of the image.
 * @property {int} channels The number of channels per pixel.
 *
 * @property {PixelFormat} [format] Pixel format of raw bytes.
 */

/**
 * Output image to a Buffer. All image processing occurs in a background thread that will not block Node's main loop. If
 * the background thread pool is full, the operation will be queued until a thread is available.
 *
 * The returned buffer has an extended field header of type Header, containing the image and Buffer details.
 *
 * If no format is specified, the default format will be bytes.
 *
 * @returns {Promise<Buffer>}
 * @method Pipeline#toBuffer
 */
function toBuffer() {
    const pixels = this;

    return new Promise(function(resolve, reject) {
        native.loadPipeline(pixels.request, false, (event, payload) => {
            if (event === 'header') {
                // ignore
            } else if (event === 'data') {
                resolve(payload);
            } else {
                reject(payload);
            }
        });
    });
}

/**
 * Output image to a Buffer. This operation occurs synchronously on Node's main thread.
 *
 * The returned buffer has an extended field header of type Header, containing the image and Buffer details.
 *
 * If no format is specified, the default format will be bytes.
 *
 * @returns {Buffer}
 * @method Pipeline#toBufferSync
 */
function toBufferSync() {
    return native.loadPipelineSync(this.request, false);
}

/**
 * Get the source image's header. The image header loading occurs in a background thread that will not block Node's main loop. If
 * the background thread pool is full, the operation will be queued until a thread is available.
 *
 * @returns {Promise<Header>}
 * @method Pipeline#toHeader
 */
function toHeader() {
    const pixels = this;

    return new Promise(function(resolve, reject) {
        native.loadPipeline(
            pixels.request,
            true,
            (event, payload) => (event === 'header') ? resolve(payload) : reject(payload));
    });
}

/**
 * Get the source image's header. This operation occurs synchronously on Node's main thread.
 *
 * @returns {Header}
 * @method Pipeline#toHeaderSync
 */
function toHeaderSync() {
    return native.loadPipelineSync(this.request, true);
}

module.exports = (Pixels) => {
    Pixels.prototype.toHeader = toHeader;
    Pixels.prototype.toHeaderSync = toHeaderSync;
    Pixels.prototype.toBuffer = toBuffer;
    Pixels.prototype.toBufferSync = toBufferSync;
};
