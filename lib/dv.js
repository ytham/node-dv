/*
 * Copyright (c) 2012 Christoph Schulz
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */
var fs = require('fs');
var path = require('path');
var binding = require(__dirname + '/dvBinding.node');

// Wrap and export Tesseract.
var Tesseract = exports.Tesseract = function(lang, image) {
    var tessdata = __dirname + '/../node_modules/dv.data/tessdata/';
    var tess;
    if (typeof lang !== 'undefined' && lang !== null
            && typeof image !== 'undefined' && image !== null) {
        tess = new binding.Tesseract(tessdata, lang, image);
    } else if (typeof lang !== 'undefined' && lang !== null) {
        tess = new binding.Tesseract(tessdata, lang);
    } else {
        tess = new binding.Tesseract(tessdata);
    }
    tess.__proto__ = Tesseract.prototype;
    return tess;
};
Tesseract.prototype = {
    __proto__: binding.Tesseract.prototype,
    constructor: Tesseract,
};

// Export others.
exports.Image = binding.Image;
exports.ZXing = binding.ZXing;
exports.Matrix = binding.Matrix;
