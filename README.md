# Pixels, Please

Image processing library for multimedia applications.

# Install

```
npm install pixels-please --save
```

```
yarn add pixels-please
```

**Pixels, Please** contains native code. The installation target must be configured to use node-gyp.

# Example

```javascript
const Pipeline = require('pixels-please');

Pipeline(imageFilename)
    .bytes({format: 'argb'})
    .toBuffer()
    .then(buffer => {
        // ...
    })
```

# Image Support

| Extension | Limitations |
|---|---|
| .jpg, .jpeg | baseline & progressive (12 bpc/arithmetic not supported, same as stock IJG lib) |
| .png | 1/2/4/8 bits per channel |
| .tga | None |
| .bmp | non-RLE, non-1bpp |
| .psd | composited view only, no extra channels, 8/16 bit-per-channel |
| .gif | animated not supported |
| .hdr | radiance rgbE format |
| .pic | None |
| .ppm, .pgm| binary only |

# Credits

**Pixels, Please** includes source code from the following projects. 

|  File |  Project |  License |
|---|---|---|
| deps/stb_image.h | https://github.com/nothings/stb | MIT |
| deps/stb_image_resize.h | https://github.com/nothings/stb | MIT  |
| deps/cptl_stl.h | https://github.com/vit-vit/CTPL | Apache 2.0 |
| deps/nanosvg.h | https://github.com/memononen/nanosvg | zlib |
| deps/nanosvgrast.h | https://github.com/memononen/nanosvg | zlib |

# License

MIT License

Copyright (C) 2018 Daniel Anderson

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation
files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy,
modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software
is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
