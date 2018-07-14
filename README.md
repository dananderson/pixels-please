# Pixels, Please

An image loading module for graphics applications.

**Pixels, Please** provides an asynchronous, non-blocking image processing pipeline that accepts an image file and outputs a Buffer of raw pixel 
data. The pipeline is intended to support the use case of loading an image file into an OpenGL texture.

```
npm install --save pixels-please
```

```
yarn add pixels-please
```

* Installation requires node-gyp.

# Examples

```javascript
const Pipeline = require('pixels-please');

Pipeline(imageFilename)
    .bytes({format: 'argb'})
    .toBuffer()
    .then(buffer => {
        // ...
    });
```

```javascript
const Pipeline = require('pixels-please');

let buffer = Pipeline(imageFilename)
    .bytes({format: 'argb'})
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

# License

Code is under the [MIT License](https://opensource.org/licenses/MIT).
