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

// Constants

#define HEADER_WIDTH "width"
#define HEADER_HEIGHT "height"
#define HEADER_CHANNELS "channels"
#define HEADER_FORMAT "format"
#define HEADER_EVENT_TYPE "header"

#define ERROR_MESSAGE "message"
#define ERROR_EVENT_TYPE "error"

#define BUFFER_HEADER "header"
#define BUFFER_EVENT_TYPE "data"

#define REQUEST_OUTPUT "outputOptions"
#define REQUEST_FORMAT "format"
#define REQUEST_SOURCE "source"
#define REQUEST_WIDTH "resizeWidth"
#define REQUEST_HEIGHT "resizeHeight"
#define REQUEST_FILTER "resizeFilter"
#define REQUEST_CONSTRAINT "resizeConstraint"
#define REQUEST_DISABLE_DECODER_SCALING "resizeDisableDecoderScaling"
#define REQUEST_IGNORE_ASPECT_RATIO "resizeIgnoreAspectRatio"

#define FILTER_BOX "box"
#define FILTER_TENT "tent"
#define FILTER_GAUSSIAN "gaussian"

#define CONSTRAINT_CONTAIN "contain"
#define CONSTRAINT_FIT "fit"

enum PixelFormat {
    PIXEL_FORMAT_RGBA = 0,
    PIXEL_FORMAT_ARGB = 1,
    PIXEL_FORMAT_ABGR = 2,
    PIXEL_FORMAT_BGRA = 3,
    PIXEL_FORMAT_RGB = 4,
    PIXEL_FORMAT_UNKNOWN = -1
};

// Exported Functions

void LoadPipeline(const CallbackInfo& info);
Value LoadPipelineSync(const CallbackInfo& info);

// Internal Functions

class ImageSource;
class Request;
class Result;

std::string PixelFormatToString(const PixelFormat pixelFormat);
PixelFormat PixelFormatFromString(const std::string& str);
int GetChannels(const PixelFormat pixelFormat);
int IsBigEndian();
bool CompletionFunction(const Value& val);
PixelFormat GetPixelFormatFromComponent(int component);
void ConvertPixelsLE(unsigned char *bytes, int len, int bytesPerPixel, PixelFormat format);
void ConvertPixelsBE(unsigned char *bytes, int len, int bytesPerPixel, PixelFormat format);
std::shared_ptr<Result> Pipeline(const std::shared_ptr<Request> request, const std::shared_ptr<ImageSource> imageSource);
float ScaleFactor(const int source, const int dest);

// Internal Classes

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

           obj[ERROR_MESSAGE] = String::New(env, this->error);

           return obj;
        }

        std::string GetError() const {
            return this->error;
        }

        std::string GetType() const {
            return ERROR_EVENT_TYPE;
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

            header[HEADER_WIDTH] = Number::New(env, this->width);
            header[HEADER_HEIGHT] = Number::New(env, this->height);
            header[HEADER_CHANNELS] = Number::New(env, this->channels);

            return header;
        }

        std::string GetType() const {
            return HEADER_EVENT_TYPE;
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

            header[HEADER_FORMAT] = String::New(env, PixelFormatToString(this->format));

            auto buffer = Napi::Buffer<unsigned char>::New(
                 env,
                 this->pixels,
                 this->width*this->height*this->channels,
                 [](Env env, void* bufferData) {
                     free(bufferData);
                 }
            );

            buffer.Set(BUFFER_HEADER, header);

            return buffer;
        }

        std::string GetType() const {
            return BUFFER_EVENT_TYPE;
        }
};

class Request {
    private:
        std::string filename;
        PixelFormat format;
        bool isHeaderQuery;

        int width;
        int height;
        std::string filter;
        std::string constraint;
        bool disableDecoderScaling;
        bool ignoreAspectRatio;

    public:
        Request(const CallbackInfo& info) {
            // Assume arguments are validated in javascript.
            auto request = info[0].As<Object>();
            auto format = request.Get(REQUEST_OUTPUT).As<Object>().Get(REQUEST_FORMAT).As<String>().Utf8Value();

            this->filename = request.Get(REQUEST_SOURCE).As<String>().Utf8Value();
            this->format = PixelFormatFromString(format);
            this->width = request.Get(REQUEST_WIDTH).As<Number>().Int32Value();
            this->height = request.Get(REQUEST_HEIGHT).As<Number>().Int32Value();
            this->filter = request.Get(REQUEST_FILTER).As<String>().Utf8Value();
            this->constraint = request.Get(REQUEST_CONSTRAINT).As<String>().Utf8Value();
            this->disableDecoderScaling = request.Get(REQUEST_DISABLE_DECODER_SCALING).As<Boolean>().Value();
            this->ignoreAspectRatio = request.Get(REQUEST_IGNORE_ASPECT_RATIO).As<Boolean>().Value();

            this->isHeaderQuery = info[1].As<Boolean>().Value();
        }

        const std::string& GetFilename() const {
            return this->filename;
        }

        PixelFormat GetFormat() const {
            return this->format;
        }

        bool IsHeaderQuery() const {
            return this->isHeaderQuery;
        }

        int GetWidth() const {
            return this->width;
        }

        int GetHeight() const {
            return this->height;
        }

        const std::string& GetFilter() const {
            return this->filter;
        }

        const std::string& GetConstraint() const {
            return this->constraint;
        }

        bool IsDisableDecoderScaling() const {
            return this->disableDecoderScaling;
        }

        bool IsIgnoreAspectRatio() const {
            return this->ignoreAspectRatio;
        }
};

class Canvas {
    private:
        float scaleX;
        float scaleY;
        int width;
        int height;
        stbir_filter filter;
        bool resize;

    public:
        Canvas(const std::shared_ptr<Request> request, const int sourceWidth, const int sourceHeight) {
            auto filter = request->GetFilter();

            if (filter == FILTER_BOX) {
                this->filter = STBIR_FILTER_BOX;
            } else if (filter == FILTER_TENT) {
                this->filter = STBIR_FILTER_TRIANGLE;
            } else if (filter == FILTER_GAUSSIAN) {
                this->filter = STBIR_FILTER_CUBICBSPLINE;
            } else {
                this->filter = STBIR_FILTER_DEFAULT;
            }

            auto destWidth = request->GetWidth();
            auto destHeight = request->GetHeight();

            this->resize = (destWidth > 0 && destHeight > 0) && !(sourceWidth == destWidth && sourceHeight == destHeight);

            // TODO: simplify if new constraints are added..
            if (this->resize) {
                if (request->GetConstraint() == "contain") {
                    if (sourceWidth <= destWidth && sourceHeight <= destHeight) {
                        // smaller than the bounding box. no resizing required.
                        this->width = sourceWidth;
                        this->height = sourceHeight;
                        this->scaleX = 1;
                        this->scaleY = 1;
                        this->resize = false;
                    } else if (request->IsIgnoreAspectRatio()) {
                        if (sourceWidth < destWidth) {
                            // width fits, squish height
                            this->width = sourceWidth;
                            this->height = destHeight;
                            this->scaleX = 1;
                            this->scaleY = ScaleFactor(sourceHeight, destHeight);
                        } else if (sourceHeight < destHeight) {
                            // height fits, squish width
                            this->width = destWidth;
                            this->height = sourceHeight;
                            this->scaleX = ScaleFactor(sourceWidth, destWidth);
                            this->scaleY = 1;
                        }
                        else {
                            // squish both dimensions
                            this->width = destWidth;
                            this->height = destHeight;
                            this->scaleX = ScaleFactor(sourceWidth, destWidth);
                            this->scaleY = ScaleFactor(sourceHeight, destHeight);
                        }
                    } else {
                        // scale by aspect ratio
                        auto aspectRatio = (float)sourceWidth / (float)sourceHeight;

                        if (sourceWidth == sourceHeight) {
                            this->width = destWidth;
                            this->height = destHeight;
                        } else if (sourceWidth > sourceHeight) {
                            this->height = (int)((float)(destWidth) / aspectRatio);
                            this->width = destWidth;
                        } else {
                            this->height = destHeight;
                            this->width = (int)((float)(destHeight) * aspectRatio);
                        }

                        this->scaleX = ScaleFactor(sourceWidth, destWidth);
                        this->scaleY = ScaleFactor(sourceHeight, destHeight);
                    }
                } else { // "fit"
                    if (request->IsIgnoreAspectRatio()) {
                        // stretch to fit
                        this->width = destWidth;
                        this->height = destHeight;
                        this->scaleX = ScaleFactor(sourceWidth, destWidth);
                        this->scaleY = ScaleFactor(sourceHeight, destHeight);
                    } else {
                        // scale by aspect ratio (same as contain..)
                        auto aspectRatio = (float)sourceWidth / (float)sourceHeight;

                        if (sourceWidth == sourceHeight) {
                            this->width = destWidth;
                            this->height = destHeight;
                        } else if (sourceWidth > sourceHeight) {
                            this->height = (int)((float)(destWidth) / aspectRatio);
                            this->width = destWidth;
                        } else {
                            this->height = destHeight;
                            this->width = (int)((float)(destHeight) * aspectRatio);
                        }

                        this->scaleX = ScaleFactor(sourceWidth, destWidth);
                        this->scaleY = ScaleFactor(sourceHeight, destHeight);
                    }
                }
            } else {
                this->width = sourceWidth;
                this->height = sourceHeight;
                this->scaleX = 1;
                this->scaleY = 1;
            }
        }

        float GetScaleX() const {
            return this->scaleX;
        }

        float GetScaleY() const {
            return this->scaleY;
        }

        int GetWidth() const {
            return this->width;
        }

        int GetHeight() const {
            return this->height;
        }

        stbir_filter GetStbFilter() const {
            return this->filter;
        }

        bool IsResize() const {
            return this->resize;
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

float ScaleFactor(const int source, const int dest) {
    return 1.f + (((float)dest - (float)source) / (float)source);
}

std::shared_ptr<Result> Pipeline(const std::shared_ptr<Request> request, const std::shared_ptr<ImageSource> imageSource) {
    // Header.
    if (!imageSource->IsLoaded()) {
        if (!imageSource->Open() || !imageSource->IsLoaded()) {
            return std::shared_ptr<Result>(new ErrorResult(imageSource->GetError()));
        }

        return std::shared_ptr<Result>(new HeaderResult(imageSource->GetWidth(), imageSource->GetHeight(), 4, request->IsHeaderQuery()));
    }

    auto width = imageSource->GetWidth();
    auto height = imageSource->GetHeight();
    auto pixelFormat = PIXEL_FORMAT_RGBA;
    auto requestedComponents = 4;
    unsigned char *pixels = nullptr;
    auto canvas = std::shared_ptr<Canvas>(new Canvas(request, width, height));

    // Load Image Data.
    if (imageSource->IsSvg()) {
        if (width <= 0 || height <= 0) {
            return std::shared_ptr<Result>(new ErrorResult("Cannot load an SVG without a width and height."));
        }

        auto rast = nsvgCreateRasterizer();

        if (rast == nullptr) {
            return std::shared_ptr<Result>(new ErrorResult(std::string("Failed to create rasterizer SVG.")));
        }

        float scaleX;
        float scaleY;

        if (request->IsDisableDecoderScaling()) {
            scaleX = 1;
            scaleY = 1;
        } else {
            scaleX = canvas->GetScaleX();
            scaleY = canvas->GetScaleY();
            width = canvas->GetWidth();
            height = canvas->GetHeight();
            pixels = (unsigned char *)malloc(width*height*requestedComponents);
        }

        pixels = (unsigned char *)malloc(width*height*requestedComponents);

        if (pixels == nullptr) {
            nsvgDeleteRasterizer(rast);
            return std::shared_ptr<Result>(new ErrorResult(std::string("Failed to allocate memory for SVG.")));
        }

        nsvgRasterizeFull(rast, imageSource->GetSvg(), 0, 0, scaleX, scaleY, pixels, width, height, width*requestedComponents);
        nsvgDeleteRasterizer(rast);
    } else {
        int components;
        
        pixels = stbi_load_from_file(imageSource->GetFile(), &width, &height, &components, requestedComponents);

        if (pixels == nullptr) {
            return std::shared_ptr<Result>(new ErrorResult(std::string("File load error: ").append(stbi_failure_reason())));
        }
    }

    // Resize.
    if (canvas->IsResize() && !(imageSource->IsSvg() && !request->IsDisableDecoderScaling())) {
        auto alphaChannelIndex = IsBigEndian() ? 3 : 0;
        auto output = (unsigned char *)malloc(canvas->GetWidth()*canvas->GetHeight()*requestedComponents);

        auto result = stbir_resize_uint8_generic(
            // input
            pixels,
            width,
            height,
            0,
            // output
            output,
            canvas->GetWidth(),
            canvas->GetHeight(),
            0,
            // channels
            requestedComponents,
            alphaChannelIndex,
            // settings
            0,
            STBIR_EDGE_CLAMP,
            canvas->GetStbFilter(),
            STBIR_COLORSPACE_LINEAR,
            // context
            nullptr
        );

        if (!result) {
            free(pixels);
            free(output);
            return std::shared_ptr<Result>(new ErrorResult(std::string("Failed to resize the image.")));
        }

        free(pixels);
        pixels = output;
        width = canvas->GetWidth();
        height = canvas->GetHeight();
    }

    // Colorspace.
    if (request->GetFormat() != PIXEL_FORMAT_UNKNOWN) {
        if (IsBigEndian()) {
            ConvertPixelsBE(pixels, width*height*requestedComponents, requestedComponents, request->GetFormat());
        } else {
            ConvertPixelsLE(pixels, width*height*requestedComponents, requestedComponents, request->GetFormat());
        }
        pixelFormat = request->GetFormat();
    }

    return std::shared_ptr<Result>(new BufferResult(width, height, GetChannels(pixelFormat), pixelFormat, pixels));
}

void LoadPipeline(const CallbackInfo& info) {
    // Assume arguments are validated in javascript.
    auto request = std::shared_ptr<Request>(new Request(info));
    auto callback = std::make_shared<ThreadSafeCallback>(info[2].As<Function>());

    GetThreadPool().push([request, callback](int id) {
        auto imageSource = std::shared_ptr<ImageSource>(new ImageSource(request->GetFilename()));

        while (true) {
            std::shared_ptr<Result> result = Pipeline(request, imageSource);

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
    // Assume arguments are validated in javascript.
    auto env = info.Env();
    auto request = std::shared_ptr<Request>(new Request(info));
    auto imageSource = std::shared_ptr<ImageSource>(new ImageSource(request->GetFilename()));
    Value returnValue;

    while (true) {
        std::shared_ptr<Result> result = Pipeline(request, imageSource);

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
