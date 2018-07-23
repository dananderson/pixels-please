# Pixels, Please

An image loading module for graphics applications.

* Supports many popular image formats.
* Image resizing.
* Pixel data conversion for 32-bit texture formats. 
* Non-blocking image loading and processing.

Tested on Windows, Mac and Linux (including Raspberry Pi).

# Requirements

* Pixels, Please contains native code and requires a system with a proper C++ toolchain (node-gyp).

# Install

```
npm install --save pixels-please
```

```
yarn add pixels-please
```

* Installation requires node-gyp.

# Examples

Asynchronously load image data into a Buffer.

```javascript
const Pipeline = require('pixels-please');

Pipeline(imageFilename)
    .bytes({format: 'argb'})
    .toBuffer()
    .then(buffer => {
        // ...
    });
```

Synchronously load image data into a Buffer in 32 bit ARGB format.

```javascript
const Pipeline = require('pixels-please');

let buffer = Pipeline(imageFilename)
    .bytes({format: 'argb'})
    .toBufferSync();
```

Read the image header for size information.

```javascript
const Pipeline = require('pixels-please');

let header = Pipeline(imageFilename)
    .toHeaderSync();

console.log(`${header.width}x${header.height}`);
```

Resize an image to fit into a 100x100 bounding box, preserving aspect ratio.

```javascript
const Pipeline = require('pixels-please');

let buffer = Pipeline(imageFilename)
    .bytes()
    .fit()
    .filter('gaussian')
    .resize(100, 100)
    .toBufferSync();
```

# Image Formats

| Extension | Limitations |
|---|---|
| .jpg | baseline & progressive (12 bpc/arithmetic not supported, same as stock IJG lib) |
| .png | 1/2/4/8 bits per channel |
| .tga | None |
| .bmp | non-RLE, non-1bpp |
| .psd | composited view only, no extra channels, 8 bit-per-channel |
| .gif | animated not supported |
| .hdr | radiance rgbE format |
| .pic | None |
| .ppm | binary only |
| .pgm | binary only |
| .svg | Supports a subset of SVG, including shapes, paths, etc. |

# License

Code is under the [MIT License](https://opensource.org/licenses/MIT).
