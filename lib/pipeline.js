/*
 * Copyright (C) 2018 Daniel Anderson
 *
 * This source code is licensed under the MIT license found in the LICENSE file
 * in the root directory of this source tree.
 */

'use strict';

const is = require('./is');

/**
 * Image processing pipeline.
 * 
 * @param {String} source Image filename to load.
 * @class
 */
function Pipeline(source) {
    if (!(this instanceof Pipeline)) {
        return new Pipeline(source);
    }

    if (!source) {
        throw Error('No image source provided.');
    }

    if (!is.string(source)) {
        throw Error('Invalid image source: ' + source);
    }

    this.request = {
        source,

        output: 'bytes',
        outputOptions: {
            format: 'keep'
        },

        resizeWidth: 0,
        resizeHeight: 0,
        resizeFilter: 'gaussian',
        resizeConstraint: 'fit',
        resizeDisableDecoderScaling: false,
        resizeIgnoreAspectRatio: false,
    };
    
    return this;
}

module.exports = Pipeline;
