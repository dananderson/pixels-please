/*
 * Copyright (C) 2018 Daniel Anderson
 *
 * This source code is licensed under the MIT license found in the LICENSE file
 * in the root directory of this source tree.
 */

#include "Pipeline.h"

#include "napi-thread-safe-callback.hpp"
#include <cstdio>
#include <cstring>
#include <cmath>

#define NANOSVG_ALL_COLOR_KEYWORDS
#define NANOSVG_IMPLEMENTATION
#include "nanosvg.h"

#define NANOSVGRAST_IMPLEMENTATION
#include "nanosvgrast.h"

#define STBI_FAILURE_USERMSG
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#define STB_IMAGE_RESIZE_IMPLEMENTATION
#include "stb_image_resize.h"

#include "Threads.h"

using namespace Napi;

enum PixelFormat {
    PIXEL_FORMAT_RGBA = 0,
    PIXEL_FORMAT_ARGB = 1,
    PIXEL_FORMAT_ABGR = 2,
    PIXEL_FORMAT_BGRA = 3,
    PIXEL_FORMAT_RGB = 4,
    PIXEL_FORMAT_UNKNOWN = -1
};

PixelFormat PixelFormatFromString(const std::string& str) {
    if (str == "rgba") {
        return PIXEL_FORMAT_RGBA;
    } else if (str == "argb") {
        return PIXEL_FORMAT_ARGB;
    } else if (str == "abgr") {
        return PIXEL_FORMAT_ABGR;
    } else if (str == "bgra") {
        return PIXEL_FORMAT_BGRA;
    }

    return PIXEL_FORMAT_UNKNOWN;
}

std::string PixelFormatToString(const PixelFormat pixelFormat) {
    switch(pixelFormat) {
        case PIXEL_FORMAT_RGBA:
            return "rgba";
        case PIXEL_FORMAT_ABGR:
            return "abgr";
        case PIXEL_FORMAT_ARGB:
            return "argb";
        case PIXEL_FORMAT_BGRA:
            return "bgra";
        case PIXEL_FORMAT_RGB:
            return "rgb";
        default:
            return "";
    }
}

int GetChannels(const PixelFormat pixelFormat) {
    switch(pixelFormat) {
        case PIXEL_FORMAT_RGBA:
        case PIXEL_FORMAT_ABGR:
        case PIXEL_FORMAT_ARGB:
        case PIXEL_FORMAT_BGRA:
            return 4;
        case PIXEL_FORMAT_RGB:
            return 3;
        default:
            return -1;
    }
}

int IsBigEndian() {
    int i = 1;
    return ! *((char *)&i);
}

bool CompletionFunction(const Value& val) {
    return val.As<Boolean>().Value();
}

void DispatchError(const std::shared_ptr<ThreadSafeCallback> callback, const std::string& message) {
    callback->call<bool>([message](Napi::Env env, std::vector<napi_value>& args) {
       Object obj = Napi::Object::New(env);

       obj["message"] = String::New(env, message);

       args.push_back(String::New(env, "error"));
       args.push_back(obj);
    },
    CompletionFunction);
}

Object CreateHeader(Env env, const int width, const int height, const int channels) {
    auto header = Object::New(env);

    header["width"] = Number::New(env, width);
    header["height"] = Number::New(env, height);
    header["channels"] = Number::New(env, channels);

    return header;
}

Object CreatePixelBufferHeader(Env env, const int width, const int height, const PixelFormat format) {
    auto header = CreateHeader(env, width, height, GetChannels(format));

    header["format"] = String::New(env, PixelFormatToString(format));

    return header;
}

void DispatchHeader(
        const std::shared_ptr<ThreadSafeCallback> callback,
        const int width,
        const int height,
        const int channels) {
    callback->call<bool>([width, height, channels](Napi::Env env, std::vector<napi_value>& args) {
           args.push_back(String::New(env, "header"));
           args.push_back(CreateHeader(env, width, height, channels));
           },
        CompletionFunction);
}

Object CreatePixelBuffer(
        Env env,
        const int width,
        const int height,
        const PixelFormat pixelFormat,
        unsigned char *pixels) {
    auto channels = GetChannels(pixelFormat);
    auto buffer = Napi::Buffer<unsigned char>::New(
         env,
         pixels,
         width*height*channels,
         [](Env env, void* bufferData) {
             delete[] static_cast<stbi_uc*>(bufferData);
         }
    );

    buffer.Set("header", CreatePixelBufferHeader(env, width, height, pixelFormat));

    return buffer;
}

void DispatchPixels(
        const std::shared_ptr<ThreadSafeCallback> callback,
        const int width,
        const int height,
        const PixelFormat format,
        unsigned char * pixels) {
    callback->call<bool>([width, height, format, pixels](Napi::Env env, std::vector<napi_value>& args) {
            args.push_back(String::New(env, "data"));
            args.push_back(CreatePixelBuffer(env, width, height, format, pixels));
        },
        CompletionFunction);
}

PixelFormat GetPixelFormatFromComponent(int component) {
    return (component == 3) ?  PIXEL_FORMAT_RGB : PIXEL_FORMAT_RGBA;
}

void ConvertPixelsLE(unsigned char *bytes, int len, int bytesPerPixel, PixelFormat format) {
    auto i = 0;
    unsigned char r, g, b, a;

    while (i < len) {
        r = bytes[i];
        g = bytes[i + 1];
        b = bytes[i + 2];
        a = (bytesPerPixel == 4) ? bytes[i + 3] : 255;

        switch(format) {
            case PIXEL_FORMAT_RGBA:
                bytes[i    ] = a;
                bytes[i + 1] = b;
                bytes[i + 2] = g;
                bytes[i + 3] = r;
                break;
            case PIXEL_FORMAT_ABGR:
                bytes[i    ] = r;
                bytes[i + 1] = g;
                bytes[i + 2] = b;
                bytes[i + 3] = a;
                break;
            case PIXEL_FORMAT_ARGB:
                bytes[i    ] = b;
                bytes[i + 1] = g;
                bytes[i + 2] = r;
                bytes[i + 3] = a;
                break;
            case PIXEL_FORMAT_BGRA:
                bytes[i    ] = a;
                bytes[i + 1] = r;
                bytes[i + 2] = g;
                bytes[i + 3] = b;
                break;
            case PIXEL_FORMAT_RGB:
                bytes[i    ] = b;
                bytes[i + 1] = g;
                bytes[i + 2] = r;
                break;
            default:
                break;
        }

        i += bytesPerPixel;
    }
}

void ConvertPixelsBE(unsigned char *bytes, int len, int bytesPerPixel, PixelFormat format) {
    auto i = 0;
    unsigned char r, g, b, a;

    while (i < len) {
        r = bytes[i];
        g = bytes[i + 1];
        b = bytes[i + 2];
        a = (bytesPerPixel == 4) ? bytes[i + 3] : 255;

        switch(format) {
            case PIXEL_FORMAT_RGBA:
                bytes[i    ] = r;
                bytes[i + 1] = g;
                bytes[i + 2] = b;
                bytes[i + 3] = a;
                break;
            case PIXEL_FORMAT_ABGR:
                bytes[i    ] = a;
                bytes[i + 1] = b;
                bytes[i + 2] = g;
                bytes[i + 3] = r;
                break;
            case PIXEL_FORMAT_ARGB:
                bytes[i    ] = a;
                bytes[i + 1] = r;
                bytes[i + 2] = g;
                bytes[i + 3] = b;
                break;
            case PIXEL_FORMAT_BGRA:
                bytes[i    ] = b;
                bytes[i + 1] = g;
                bytes[i + 2] = r;
                bytes[i + 3] = a;
                break;
            case PIXEL_FORMAT_RGB:
                bytes[i    ] = r;
                bytes[i + 1] = g;
                bytes[i + 2] = b;
                break;
            default:
                break;
        }

        i += bytesPerPixel;
    }
}

void LoadPixels(const std::shared_ptr<ThreadSafeCallback> callback, const std::string& filename, const bool isHeaderQuery, const PixelFormat targetPixelFormat) {
    auto fp = fopen(filename.c_str(), "rb");

    if (fp == NULL) {
        DispatchError(callback, "File not found.");
        return;
    }

    auto width = 0;
    auto height = 0;
    auto components = 0;

    if (stbi_info_from_file(fp, &width, &height, &components) != 1) {
        DispatchError(callback, std::string("File read error: ").append(stbi_failure_reason()));
        fclose(fp);
        return;
    }

    if (callback != nullptr) {
        DispatchHeader(callback, width, height, 4);
    }

    if (isHeaderQuery) {
        return;
    }

    // TODO: Support 24-bit RGB. Until then, force full 32-bit.
    auto pixelFormat = PIXEL_FORMAT_RGBA;
    auto requestedComponents = 4;
    auto pixels = stbi_load_from_file(fp, &width, &height, &components, requestedComponents);

    if (pixels == NULL) {
        DispatchError(callback, std::string("File load error: ").append(stbi_failure_reason()));
        fclose(fp);
        return;
    }

    // TODO: resize operations go here

    if (pixelFormat != targetPixelFormat && targetPixelFormat != PIXEL_FORMAT_UNKNOWN) {
        if (IsBigEndian()) {
            ConvertPixelsBE(pixels, width*height*requestedComponents, requestedComponents, targetPixelFormat);
        } else {
            ConvertPixelsLE(pixels, width*height*requestedComponents, requestedComponents, targetPixelFormat);
        }
        pixelFormat = targetPixelFormat;
    }

    if (callback != nullptr) {
        DispatchPixels(callback, width, height, pixelFormat, pixels);
    }
    
    fclose(fp);
}

void LoadPipeline(const CallbackInfo& info) {
    // Assume parameters are checked in javascript.
    auto request = info[0].As<Object>();
    auto isHeaderQuery = info[1].As<Boolean>().Value();
    auto callback = std::make_shared<ThreadSafeCallback>(info[2].As<Function>());

    auto filename = request.Get("source").As<String>().Utf8Value();
    auto pixelFormatStr = request.Get("outputOptions").As<Object>().Get("format").As<String>().Utf8Value();
    auto pixelFormat = PixelFormatFromString(pixelFormatStr);

    GetThreadPool().push([callback, filename, isHeaderQuery, pixelFormat](int id) {
        LoadPixels(callback, filename, isHeaderQuery, pixelFormat);
    });
}

// TODO: Refactor LoadPipeline and LoadPipelineSync to eliminate duplicate logic.

Value LoadPipelineSync(const CallbackInfo& info) {
    // Assume parameters are checked in javascript.
    auto env = info.Env();
    auto request = info[0].As<Object>();
    auto isHeaderQuery = info[1].As<Boolean>().Value();

    auto filenameValue = request.Get("source").As<String>();
    auto filename = filenameValue.Utf8Value();
    auto targetPixelFormatStr = request.Get("outputOptions").As<Object>().Get("format").As<String>().Utf8Value();
    auto targetPixelFormat = PixelFormatFromString(targetPixelFormatStr);

    int width;
    int height;
    int components;
    int requestedComponents = 4;

    if (isHeaderQuery) {
        if (stbi_info(filename.c_str(), &width, &height, &components) != 1) {
            Napi::Error::New(env, std::string("File read error: ").append(stbi_failure_reason())).ThrowAsJavaScriptException();
            return env.Null();
        }

        // TODO: Support 24-bit RGB. Until then, force full 32-bit.
        return CreateHeader(env, width, height, 4);
    }

    auto pixels = stbi_load(filename.c_str(), &width, &height, &components, requestedComponents);
    auto pixelFormat = PIXEL_FORMAT_RGBA;

    if (pixels == NULL) {
        Napi::Error::New(env, "Failed to load image file").ThrowAsJavaScriptException();
        return env.Null();
    }

    // TODO: resize operations go here

    if (targetPixelFormat != PIXEL_FORMAT_UNKNOWN) {
        if (IsBigEndian()) {
            ConvertPixelsBE(pixels, width*height*requestedComponents, requestedComponents, targetPixelFormat);
        } else {
            ConvertPixelsLE(pixels, width*height*requestedComponents, requestedComponents, targetPixelFormat);
        }

        pixelFormat = targetPixelFormat;
    }

    return CreatePixelBuffer(env, width, height, pixelFormat, pixels);
}
