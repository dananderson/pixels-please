/*
 * Copyright (C) 2018 Daniel Anderson
 *
 * This source code is licensed under the MIT license found in the LICENSE file
 * in the root directory of this source tree.
 */

'use strict';

/**
 * Supported pixel formats.
 * 
 * @typedef {('rgba'|'argb'|'abgr'|'bgra'|'keep')} PixelFormat
 */
const gPixelFormats = new Set(['rgba', 'argb', 'abgr', 'bgra', 'keep']);

/**
 * Configures the pipeline to output the image as raw bytes.
 *
 * @arg {Object} [options]
 * @arg {PixelFormat} options.format The pixel format of the raw bytes.
 * @returns {Image} 
 * @method Pipeline#bytes
 */
function bytes(options) {
    this.request.output = 'bytes';

    if (options) {
        if ('format' in options && !gPixelFormats.has(options.format)) {
            throw Error('Invalid pixel format option: ' + options.format + '. Valid values: ' + Array.from(gPixelFormats).join(', '));
        }

        this.request.outputOptions.format = options.format;
    }

    return this;
}

module.exports = (Pixels) => {
    Pixels.prototype.bytes = bytes;
};
