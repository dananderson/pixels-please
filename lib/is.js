/*
 * Copyright (C) 2018 Daniel Anderson
 *
 * This source code is licensed under the MIT license found in the LICENSE file
 * in the root directory of this source tree.
 */

const number = p => typeof p === 'number' && !Number.isNaN(p);
const string = p => typeof p === 'string' && p.length > 0;
const int = p => number(p) && p % 1 === 0;

module.exports = {
    string,
    number,
    int,
};
