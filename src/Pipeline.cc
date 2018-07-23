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

std::string PixelFormatToString(const PixelFormat pixelFormat);
PixelFormat PixelFormatFromString(const std::string& str);

class ImageSource {
    private:
        FILE *file;
        std::string filename;
        std::string error;
        NSVGimage *svg;

        int width;
        int height;
        int channels;

    public:
        ImageSource(const std::string& filename) {
            this->filename = filename;
            this->file = nullptr;
            this->svg = nullptr;
            this->width = this->height = this->channels = 0;
        }

        bool Open() {
            this->file = fopen((this->filename).c_str(), "rb");

            if (this->file == nullptr) {
                this->error = "File not found.";
                return false;
            }

            if (stbi_info_from_file(this->file, &(this->width), &(this->height), &(this->channels)) != 1) {
                static auto bufferSize = 4096;
                char buffer[bufferSize];
                size_t bytesRead;

                bytesRead = fread(buffer, 1, bufferSize - 1, this->file);
                fseek(this->file, 0, SEEK_SET);
                
                if (bytesRead) {
                    buffer[bufferSize - 1] = '\0';

                    if (strstr(buffer, "<svg") == nullptr) {
                        this->error = std::string("File read error: ").append(stbi_failure_reason());
                        return false;
                    }
                } else {
                    this->error = std::string("File read error: ").append(stbi_failure_reason());
                    return false;
                }

                this->svg = nsvgParseFromFile((this->filename).c_str(), "px", 96);

                if (this->svg == nullptr) {
                    this->error = std::string("Failed to parse SVG.");
                    return false;
                }

                this->width = this->svg->width;
                this->height = this->svg->height;
                this->channels = 4;
            }

            return true;
        }

        void Close() {
            if (this->file) {
                fclose(this->file);
                this->file = nullptr;
            }

            if (this->svg) {
                nsvgDelete(this->svg);
                this->svg = nullptr;
            }
        }

        bool IsLoaded() const {
            return this->file || this->svg;
        }

        bool IsSvg() const {
            return this->svg != nullptr;
        }

        NSVGimage *GetSvg() const {
            return this->svg;
        }

        int GetWidth() const {
            return this->width;
        }

        int GetHeight() const {
            return this->height;
        }

        int GetChannels() const {
            return this->channels;
        }

        const std::string& GetError() const {
            return this->error;
        }

        FILE *GetFile() const {
            return this->file;
        }
};

class Result {
private:
    bool final;

public:
    Result(bool final) {
        this->final = final;
    }

    virtual ~Result() {

    }

    virtual Value ToValue(Env env) const = 0;

    virtual std::string GetType() const = 0;

    bool IsFinal() const {
        return this->final;
    }
};

class ErrorResult : public Result {
    private:
        std::string error;

    public:
        ErrorResult(const std::string& error) : Result(true) {
            this->error = error;
        }

        Value ToValue(Env env) const {
           Object obj = Napi::Object::New(env);

           obj["message"] = String::New(env, this->error);

           return obj;
        }

        std::string GetError() const {
            return this->error;
        }

        std::string GetType() const {
            return "error";
        }
};

class HeaderResult : public Result {
    protected:
        int width;
        int height;
        int channels;

    public:
        HeaderResult(const int width, const int height, const int channels, const bool final) : Result(final) {
            this->width = width;
            this->height = height;
            this->channels = channels;
        }

        Value ToValue(Env env) const {
            auto header = Object::New(env);

            header["width"] = Number::New(env, this->width);
            header["height"] = Number::New(env, this->height);
            header["channels"] = Number::New(env, this->channels);

            return header;
        }

        std::string GetType() const {
            return "header";
        }
};

class BufferResult : public HeaderResult {
    private:
        PixelFormat format;
        unsigned char * pixels;

    public:
        BufferResult(const int width, const int height, const int channels, const PixelFormat format, unsigned char * pixels)
                : HeaderResult(width, height, channels, true) {
            this->format = format;
            this->pixels = pixels;
        }

        Value ToValue(Env env) const {
            auto header = HeaderResult::ToValue(env).As<Object>();

            header["format"] = String::New(env, PixelFormatToString(this->format));

            auto buffer = Napi::Buffer<unsigned char>::New(
                 env,
                 this->pixels,
                 this->width*this->height*this->channels,
                 [](Env env, void* bufferData) {
                     delete[] static_cast<unsigned char*>(bufferData);
                 }
            );

            buffer.Set("header", header);

            return buffer;
        }

        std::string GetType() const {
            return "data";
        }
};

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

std::shared_ptr<Result> Pipeline(const std::shared_ptr<ImageSource> imageSource, const bool isHeaderQuery, const PixelFormat targetPixelFormat) {
    if (!imageSource->IsLoaded()) {
        if (!imageSource->Open() || !imageSource->IsLoaded()) {
            return std::shared_ptr<Result>(new ErrorResult(imageSource->GetError()));
        }

        return std::shared_ptr<Result>(new HeaderResult(imageSource->GetWidth(), imageSource->GetHeight(), 4, isHeaderQuery));
    }

    auto width = imageSource->GetWidth();
    auto height = imageSource->GetHeight();
    auto pixelFormat = PIXEL_FORMAT_RGBA;
    auto requestedComponents = 4;
    unsigned char *pixels = nullptr;

    if (imageSource->IsSvg()) {
        if (width <= 0 || height <= 0) {
            return std::shared_ptr<Result>(new ErrorResult("Cannot load an SVG without a width and height."));
        }

        auto rast = nsvgCreateRasterizer();

        if (rast == nullptr) {
            return std::shared_ptr<Result>(new ErrorResult(std::string("Failed to create rasterizer SVG.")));
        }

        pixels = new unsigned char[width*height*requestedComponents];

        if (pixels == nullptr) {
            nsvgDeleteRasterizer(rast);
            return std::shared_ptr<Result>(new ErrorResult(std::string("Failed to allocate memory for SVG.")));
        }

        nsvgRasterize(rast, imageSource->GetSvg(), 0, 0, 1, pixels, width, height, width*requestedComponents);
        nsvgDeleteRasterizer(rast);
    } else {
        int components;
        
        pixels = stbi_load_from_file(imageSource->GetFile(), &width, &height, &components, requestedComponents);

        if (pixels == nullptr) {
            return std::shared_ptr<Result>(new ErrorResult(std::string("File load error: ").append(stbi_failure_reason())));
        }
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

    return std::shared_ptr<Result>(new BufferResult(width, height, GetChannels(pixelFormat), pixelFormat, pixels));
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
        std::shared_ptr<ImageSource> imageSource = std::shared_ptr<ImageSource>(new ImageSource(filename));

        while (true) {
            std::shared_ptr<Result> result = Pipeline(imageSource, isHeaderQuery, pixelFormat);

            callback->call<bool>([result](Napi::Env env, std::vector<napi_value>& args) {
                args.push_back(String::New(env, result->GetType()));
                args.push_back(result->ToValue(env));
            },
            CompletionFunction);

            if (result->IsFinal()) {
                break;
            }
        }

        imageSource->Close();
    });
}

Value LoadPipelineSync(const CallbackInfo& info) {
    // Assume parameters are checked in javascript.
    auto env = info.Env();
    auto request = info[0].As<Object>();
    auto isHeaderQuery = info[1].As<Boolean>().Value();

    auto filenameValue = request.Get("source").As<String>();
    auto filename = filenameValue.Utf8Value();
    auto targetPixelFormatStr = request.Get("outputOptions").As<Object>().Get("format").As<String>().Utf8Value();
    auto targetPixelFormat = PixelFormatFromString(targetPixelFormatStr);

    std::shared_ptr<ImageSource> imageSource = std::shared_ptr<ImageSource>(new ImageSource(filename));
    Value returnValue;

    while (true) {
        std::shared_ptr<Result> result = Pipeline(imageSource, isHeaderQuery, targetPixelFormat);

        returnValue = result->ToValue(env);

        if (result->GetType() == "error") {
            Napi::Error::New(env, std::static_pointer_cast<ErrorResult>(result)->GetError()).ThrowAsJavaScriptException();
            return env.Null();
        }

        if (result->IsFinal()) {
            break;
        }
    }

    imageSource->Close();

    return returnValue;
}
