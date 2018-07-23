/*
 * Copyright (C) 2018 Daniel Anderson
 *
 * This source code is licensed under the MIT license found in the LICENSE file
 * in the root directory of this source tree.
 */

'use strict';

const Pipeline = require('./pipeline');

require('./format')(Pipeline);
require('./output')(Pipeline);
require('./config')(Pipeline);
require('./resize')(Pipeline);

module.exports = Pipeline;
