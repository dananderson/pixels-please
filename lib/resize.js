/*
 * Copyright (C) 2018 Daniel Anderson
 *
 * This source code is licensed under the MIT license found in the LICENSE file
 * in the root directory of this source tree.
 */

'use strict';

const is = require('./is');

/**
 * Supported resize filter formats.
 *
 * @typedef {('box'|'tent'|'gaussian')} Filter
 */
const gFilters = new Set(['box', 'tent', 'gaussian']);

/**
 * Resize an image.
 *
 * The width and height may not be the final output dimensions, but an upper bound of a resize operation. The width
 * and height create a resizing bounding box.
 *
 * @param width Maximum output width.
 * @param height Maximum output height.
 * @returns {Pipeline}
 * @method Pipeline#resize
 */
function resize(width, height) {
    if (!is.int(width) || width <= 0) {
        throw Error(`Invalid width of ${width}. Should be a positive integer.`);
    }

    if (!is.int(height) || height <= 0) {
        throw Error(`Invalid height of ${height}. Should be a positive integer.`);
    }

    this.request.resizeWidth = width;
    this.request.resizeHeight = height;

    return this;
}

/**
 * Resize an image only if it's width or height are larger that the resize bounding box.
 *
 * @returns {Pipeline}
 * @method Pipeline#contain
 */
function contain() {
    this.request.resizeConstraint = 'contain';
    return this;
}

/**
 * Resize an image to fill as much of the resize bounding box as possible.
 *
 * @returns {Pipeline}
 * @method Pipeline#fit
 */
function fit() {
    this.request.resizeConstraint = 'fit';
    return this;
}

/**
 * The resize algorithm to use.
 *
 * Note, some image decoders support scaling during decoding (such as SVG). If available, this will be the preferred
 * resizing strategy. If disableDecoderScaling is specified in options, the image will be loaded at it's normal
 * size and resized with this filter.
 *
 * @arg {Filter} filter Resize filter algorithm.
 * @arg options
 * @arg {boolean} options.disableDecoderScaling Force the resize operation to use the specified resize filter.
 * @returns {Pipeline}
 * @method Pipeline#filter
 */
function filter(filter, options) {
    if (!gFilters.has(filter)) {
        throw Error(`Invalid resize filter option: ${options.format}. Valid values: ${Array.from(gFilters).join(', ')}`);
    }

    this.request.resizeFilter = filter;
    options && (this.request.resizeDisableDecoderScaling = !!options.disableDecoderScaling);

    return this;
}

/**
 * Ignore image aspect ratio when resizing.
 * 
 * @returns {Pipeline}
 * @method Pipeline#ignoreAspectRatio
 */
function ignoreAspectRatio() {
    this.request.resizeIgnoreAspectRatio = false;

    return this;
}

module.exports = (Pixels) => {
    Object.assign(Pixels.prototype, { resize, contain, fit, filter, ignoreAspectRatio });
};
