#include <cstring>
#include <cmath>
#include <cstdint>
#include "ofxImageEffect.h"
#include "ofxMultiThread.h"

#if defined _WIN32
  #define EXPORT OfxExport
#else
  #define EXPORT __attribute__((visibility("default")))
#endif

static OfxHost* gHost = NULL;
static OfxImageEffectSuiteV1* gEffectHost = NULL;
static OfxPropertySuiteV1* gPropHost = NULL;
static OfxParameterSuiteV1* gParamHost = NULL;
static OfxMultiThreadSuiteV1* gThreadHost = NULL;

static constexpr int kQualityFast = 0;
static constexpr int kQualityHigh = 1;
static constexpr int kNoiseSmooth = 0;
static constexpr int kNoiseRidged = 1;

static inline int clampInt(int value, int lo, int hi) {
    return value < lo ? lo : (value > hi ? hi : value);
}

static OfxStatus onLoad(void) {
    if (!gHost) return kOfxStatErrMissingHostFeature;
    gEffectHost = (OfxImageEffectSuiteV1*)gHost->fetchSuite(gHost->host, kOfxImageEffectSuite, 1);
    gPropHost = (OfxPropertySuiteV1*)gHost->fetchSuite(gHost->host, kOfxPropertySuite, 1);
    gParamHost = (OfxParameterSuiteV1*)gHost->fetchSuite(gHost->host, kOfxParameterSuite, 1);
    gThreadHost = (OfxMultiThreadSuiteV1*)gHost->fetchSuite(gHost->host, kOfxMultiThreadSuite, 1);
    if (!gEffectHost || !gPropHost || !gParamHost) return kOfxStatErrMissingHostFeature;
    return kOfxStatOK;
}

static OfxStatus describe(OfxImageEffectHandle effect) {
    OfxPropertySetHandle props;
    gEffectHost->getPropertySet(effect, &props);
    gPropHost->propSetString(props, kOfxPropLabel, 0, "Boilify");
    gPropHost->propSetString(props, kOfxImageEffectPluginPropGrouping, 0, "Distort");
    gPropHost->propSetInt(props, kOfxImageEffectPropSupportsMultipleClipDepths, 0, 0);
    gPropHost->propSetString(props, kOfxImageEffectPropSupportedPixelDepths, 0, kOfxBitDepthByte);
    gPropHost->propSetString(props, kOfxImageEffectPropSupportedPixelDepths, 1, kOfxBitDepthShort);
    gPropHost->propSetString(props, kOfxImageEffectPropSupportedPixelDepths, 2, kOfxBitDepthFloat);
    gPropHost->propSetString(props, kOfxImageEffectPropSupportedContexts, 0, kOfxImageEffectContextFilter);
    return kOfxStatOK;
}

static OfxStatus describeInContext(OfxImageEffectHandle effect, OfxPropertySetHandle) {
    OfxPropertySetHandle props;

    gEffectHost->clipDefine(effect, kOfxImageEffectOutputClipName, &props);
    gPropHost->propSetString(props, kOfxImageEffectPropSupportedComponents, 0, kOfxImageComponentRGBA);

    gEffectHost->clipDefine(effect, kOfxImageEffectSimpleSourceClipName, &props);
    gPropHost->propSetString(props, kOfxImageEffectPropSupportedComponents, 0, kOfxImageComponentRGBA);

    OfxParamSetHandle params;
    gEffectHost->getParamSet(effect, &params);

    gParamHost->paramDefine(params, kOfxParamTypeDouble, "strength", &props);
    gPropHost->propSetString(props, kOfxPropLabel, 0, "Strength");
    gPropHost->propSetDouble(props, kOfxParamPropDefault, 0, 5.0);
    gPropHost->propSetDouble(props, kOfxParamPropMin, 0, 0.0);
    gPropHost->propSetDouble(props, kOfxParamPropMax, 0, 100.0);
    gPropHost->propSetDouble(props, kOfxParamPropDisplayMin, 0, 0.0);
    gPropHost->propSetDouble(props, kOfxParamPropDisplayMax, 0, 20.0);

    gParamHost->paramDefine(params, kOfxParamTypeDouble, "size", &props);
    gPropHost->propSetString(props, kOfxPropLabel, 0, "Size");
    gPropHost->propSetDouble(props, kOfxParamPropDefault, 0, 30.0);
    gPropHost->propSetDouble(props, kOfxParamPropMin, 0, 1.0);
    gPropHost->propSetDouble(props, kOfxParamPropMax, 0, 600.0);
    gPropHost->propSetDouble(props, kOfxParamPropDisplayMin, 0, 5.0);
    gPropHost->propSetDouble(props, kOfxParamPropDisplayMax, 0, 150.0);

    gParamHost->paramDefine(params, kOfxParamTypeDouble, "speed", &props);
    gPropHost->propSetString(props, kOfxPropLabel, 0, "Speed");
    gPropHost->propSetDouble(props, kOfxParamPropDefault, 0, 1.0);
    gPropHost->propSetDouble(props, kOfxParamPropMin, 0, 0.0);
    gPropHost->propSetDouble(props, kOfxParamPropMax, 0, 6.0);
    gPropHost->propSetDouble(props, kOfxParamPropDisplayMin, 0, 0.0);
    gPropHost->propSetDouble(props, kOfxParamPropDisplayMax, 0, 6.0);

    gParamHost->paramDefine(params, kOfxParamTypeInteger, "boilFps", &props);
    gPropHost->propSetString(props, kOfxPropLabel, 0, "FPS");
    gPropHost->propSetInt(props, kOfxParamPropDefault, 0, 12);
    gPropHost->propSetInt(props, kOfxParamPropMin, 0, 1);
    gPropHost->propSetInt(props, kOfxParamPropMax, 0, 48);
    gPropHost->propSetInt(props, kOfxParamPropDisplayMin, 0, 1);
    gPropHost->propSetInt(props, kOfxParamPropDisplayMax, 0, 24);

    gParamHost->paramDefine(params, kOfxParamTypeInteger, "complexity", &props);
    gPropHost->propSetString(props, kOfxPropLabel, 0, "Complexity");
    gPropHost->propSetInt(props, kOfxParamPropDefault, 0, 3);
    gPropHost->propSetInt(props, kOfxParamPropMin, 0, 1);
    gPropHost->propSetInt(props, kOfxParamPropMax, 0, 6);
    gPropHost->propSetInt(props, kOfxParamPropDisplayMin, 0, 1);
    gPropHost->propSetInt(props, kOfxParamPropDisplayMax, 0, 6);

    gParamHost->paramDefine(params, kOfxParamTypeInteger, "seed", &props);
    gPropHost->propSetString(props, kOfxPropLabel, 0, "Seed");
    gPropHost->propSetInt(props, kOfxParamPropDefault, 0, 0);
    gPropHost->propSetInt(props, kOfxParamPropMin, 0, 0);
    gPropHost->propSetInt(props, kOfxParamPropMax, 0, 9999);
    gPropHost->propSetInt(props, kOfxParamPropDisplayMin, 0, 0);
    gPropHost->propSetInt(props, kOfxParamPropDisplayMax, 0, 9999);

    gParamHost->paramDefine(params, kOfxParamTypeChoice, "noise", &props);
    gPropHost->propSetString(props, kOfxPropLabel, 0, "Noise");
    gPropHost->propSetInt(props, kOfxParamPropDefault, 0, kNoiseSmooth);
    gPropHost->propSetString(props, kOfxParamPropChoiceOption, 0, "Smooth");
    gPropHost->propSetString(props, kOfxParamPropChoiceOption, 1, "Ridged");

    gParamHost->paramDefine(params, kOfxParamTypeChoice, "quality", &props);
    gPropHost->propSetString(props, kOfxPropLabel, 0, "Quality");
    gPropHost->propSetInt(props, kOfxParamPropDefault, 0, kQualityFast);
    gPropHost->propSetString(props, kOfxParamPropChoiceOption, 0, "Fast");
    gPropHost->propSetString(props, kOfxParamPropChoiceOption, 1, "High");

    gParamHost->paramDefine(params, kOfxParamTypeBoolean, "animate", &props);
    gPropHost->propSetString(props, kOfxPropLabel, 0, "Animate");
    gPropHost->propSetInt(props, kOfxParamPropDefault, 0, 1);

    return kOfxStatOK;
}

static OfxStatus createInstance(OfxImageEffectHandle effect, OfxPropertySetHandle) {
    return kOfxStatOK;
}

static OfxStatus destroyInstance(OfxImageEffectHandle effect, OfxPropertySetHandle) {
    return kOfxStatOK;
}

static inline int fastFloor(float value) {
    int base = (int)value;
    return base - (base > value);
}

static inline uint32_t mixBits(uint32_t value) {
    value ^= value >> 16;
    value *= 0x7feb352dU;
    value ^= value >> 15;
    value *= 0x846ca68bU;
    value ^= value >> 16;
    return value;
}

static inline float hashFast(int x, int y, int seed) {
    uint32_t h = (uint32_t)x * 0x1f123bb5U;
    h ^= (uint32_t)y * 0x59d2f15dU;
    h ^= (uint32_t)seed * 0x6c8e9cf5U;
    h = mixBits(h);
    return (float)(h & 0x00ffffffU) * (1.0f / 16777216.0f);
}

static float perlinFast(float x, float y, int seed) {
    int x0 = fastFloor(x);
    int y0 = fastFloor(y);
    float fx = x - (float)x0;
    float fy = y - (float)y0;
    float sx = fx * fx * (3.0f - 2.0f * fx);
    float sy = fy * fy * (3.0f - 2.0f * fy);
    float n00 = hashFast(x0, y0, seed);
    float n10 = hashFast(x0 + 1, y0, seed);
    float n01 = hashFast(x0, y0 + 1, seed);
    float n11 = hashFast(x0 + 1, y0 + 1, seed);
    return n00 + sx * (n10 - n00) + sy * (n01 + sx * (n11 - n01) - n00 - sx * (n10 - n00));
}

static inline float applyNoiseType(float value, int noiseType) {
    if (noiseType != kNoiseRidged) return value;
    float centered = 2.0f * value - 1.0f;
    return 1.0f - fabsf(centered);
}

static float fbm(float x, float y, int seed, int octaves, int noiseType) {
    float sum = 0.0f;
    float amp = 1.0f;
    float ampSum = 0.0f;
    float freq = 1.0f;

    for (int i = 0; i < octaves; ++i) {
        float n = applyNoiseType(perlinFast(x * freq, y * freq, seed + i * 101), noiseType);
        sum += n * amp;
        ampSum += amp;
        amp *= 0.5f;
        freq *= 2.0f;
    }

    return (ampSum > 0.0f) ? (sum / ampSum) : 0.0f;
}

static inline float valueNoiseFast(float x, float y, int seed) {
    const int xi = fastFloor(x);
    const int yi = fastFloor(y);
    return hashFast(xi, yi, seed);
}

static float fbmPreviewFast(float x, float y, int seed, int octaves, int noiseType) {
    const int cappedOctaves = clampInt(octaves, 1, 2);
    float sum = 0.0f;
    float amp = 1.0f;
    float ampSum = 0.0f;
    float freq = 1.0f;

    for (int i = 0; i < cappedOctaves; ++i) {
        float n = applyNoiseType(valueNoiseFast(x * freq, y * freq, seed + i * 101), noiseType);
        sum += n * amp;
        ampSum += amp;
        amp *= 0.5f;
        freq *= 2.0f;
    }

    return (ampSum > 0.0f) ? (sum / ampSum) : 0.0f;
}

struct RenderArgs {
    OfxImageEffectHandle instance;
    char* srcPtr;
    char* dstPtr;
    OfxRectI srcRect;
    OfxRectI dstRect;
    OfxRectI window;
    int srcRowBytes;
    int dstRowBytes;
    int pixelBytes;
    float strength;
    float invSize;
    int seedX;
    int seedY;
    int complexity;
    int noiseType;
    int qualityMode;
};

static void renderSlice(unsigned int threadId, unsigned int nThreads, void* vargs) {
    RenderArgs* args = (RenderArgs*)vargs;

    int dy = args->window.y2 - args->window.y1;
    int y1 = args->window.y1 + threadId * dy / nThreads;
    int y2 = args->window.y1 + ((threadId + 1) * dy / nThreads < dy ? (threadId + 1) * dy / nThreads : dy);

    for (int y = y1; y < y2; y++) {
        char* dstRow = args->dstPtr + (ptrdiff_t)(y - args->dstRect.y1) * args->dstRowBytes;
        float ny = (float)(y - args->dstRect.y1) * args->invSize;

        for (int x = args->window.x1; x < args->window.x2; x++) {
            int dstX = x - args->dstRect.x1;

            int srcX = x;
            int srcY = y;

            char* origSrcRow = args->srcPtr + (ptrdiff_t)(y - args->srcRect.y1) * args->srcRowBytes;
            char* origSrcPix = origSrcRow + (ptrdiff_t)dstX * args->pixelBytes;
            float alpha = 1.0f;
            if (args->pixelBytes == 4) {
                alpha = (float)((unsigned char)origSrcPix[3]) / 255.0f;
            } else if (args->pixelBytes == 8) {
                alpha = ((unsigned short*)origSrcPix)[3] / 65535.0f;
            } else if (args->pixelBytes == 16) {
                alpha = ((float*)origSrcPix)[3];
            }

            if (alpha > 0.0f) {
                float nx = (float)dstX * args->invSize;
                const float fxNoise = (args->qualityMode == kQualityFast)
                    ? fbmPreviewFast(nx, ny, args->seedX, args->complexity, args->noiseType)
                    : fbm(nx, ny, args->seedX, args->complexity, args->noiseType);
                const float fyNoise = (args->qualityMode == kQualityFast)
                    ? fbmPreviewFast(nx, ny, args->seedY, args->complexity, args->noiseType)
                    : fbm(nx, ny, args->seedY, args->complexity, args->noiseType);
                float fx = fxNoise * 2.0f - 1.0f;
                float fy = fyNoise * 2.0f - 1.0f;
                srcX = x + (int)(fx * args->strength);
                srcY = y + (int)(fy * args->strength);
            }

            srcX = srcX < args->srcRect.x1 ? args->srcRect.x1 : (srcX >= args->srcRect.x2 ? args->srcRect.x2 - 1 : srcX);
            srcY = srcY < args->srcRect.y1 ? args->srcRect.y1 : (srcY >= args->srcRect.y2 ? args->srcRect.y2 - 1 : srcY);

            char* srcRow = args->srcPtr + (ptrdiff_t)(srcY - args->srcRect.y1) * args->srcRowBytes;
            std::memcpy(dstRow + (ptrdiff_t)dstX * args->pixelBytes,
                        srcRow + (ptrdiff_t)(srcX - args->srcRect.x1) * args->pixelBytes,
                        (size_t)args->pixelBytes);
        }
    }
}

static OfxStatus render(OfxImageEffectHandle instance, OfxPropertySetHandle inArgs, OfxPropertySetHandle) {
    OfxTime time;
    OfxRectI window;
    gPropHost->propGetDouble(inArgs, kOfxPropTime, 0, &time);
    gPropHost->propGetIntN(inArgs, kOfxImageEffectPropRenderWindow, 4, &window.x1);

    OfxImageClipHandle srcClip, dstClip;
    gEffectHost->clipGetHandle(instance, kOfxImageEffectSimpleSourceClipName, &srcClip, NULL);
    gEffectHost->clipGetHandle(instance, kOfxImageEffectOutputClipName, &dstClip, NULL);

    OfxPropertySetHandle srcImg = NULL, dstImg = NULL;
    if (gEffectHost->clipGetImage(dstClip, time, NULL, &dstImg) != kOfxStatOK) return kOfxStatFailed;
    if (gEffectHost->clipGetImage(srcClip, time, NULL, &srcImg) != kOfxStatOK) {
        gEffectHost->clipReleaseImage(dstImg);
        return kOfxStatFailed;
    }

    OfxParamSetHandle paramSet;
    gEffectHost->getParamSet(instance, &paramSet);

    OfxParamHandle strengthParam, sizeParam, speedParam, seedParam, animateParam, qualityParam;
    OfxParamHandle boilFpsParam, complexityParam, noiseParam;
    gParamHost->paramGetHandle(paramSet, "strength", &strengthParam, NULL);
    gParamHost->paramGetHandle(paramSet, "size", &sizeParam, NULL);
    gParamHost->paramGetHandle(paramSet, "speed", &speedParam, NULL);
    gParamHost->paramGetHandle(paramSet, "seed", &seedParam, NULL);
    gParamHost->paramGetHandle(paramSet, "animate", &animateParam, NULL);
    gParamHost->paramGetHandle(paramSet, "quality", &qualityParam, NULL);
    gParamHost->paramGetHandle(paramSet, "boilFps", &boilFpsParam, NULL);
    gParamHost->paramGetHandle(paramSet, "complexity", &complexityParam, NULL);
    gParamHost->paramGetHandle(paramSet, "noise", &noiseParam, NULL);

    double strength, size, speed;
    int seed, animate, qualityMode, boilFps, complexity, noiseType;
    gParamHost->paramGetValue(strengthParam, &strength);
    gParamHost->paramGetValue(sizeParam, &size);
    gParamHost->paramGetValue(speedParam, &speed);
    gParamHost->paramGetValue(seedParam, &seed);
    gParamHost->paramGetValue(animateParam, &animate);
    gParamHost->paramGetValue(qualityParam, &qualityMode);
    gParamHost->paramGetValue(boilFpsParam, &boilFps);
    gParamHost->paramGetValue(complexityParam, &complexity);
    gParamHost->paramGetValue(noiseParam, &noiseType);

    OfxPropertySetHandle effectProps = NULL;
    double frameRate = 24.0;
    if (gEffectHost->getPropertySet(instance, &effectProps) == kOfxStatOK && effectProps) {
        gPropHost->propGetDouble(effectProps, kOfxImageEffectPropFrameRate, 0, &frameRate);
    }
    if (!(frameRate > 0.0)) frameRate = 24.0;

    const double seconds = time / frameRate;
    const double fps = (boilFps < 1) ? 1.0 : (double)boilFps;
    const int stepIndex = animate ? (int)floor(seconds * fps * (speed > 0.0 ? speed : 0.0)) : 0;
    const int stepWrapped = ((stepIndex % 32768) + 32768) % 32768;

    int srcRowBytes, dstRowBytes;
    OfxRectI srcRect, dstRect;
    void *srcPtr, *dstPtr;
    gPropHost->propGetInt(srcImg, kOfxImagePropRowBytes, 0, &srcRowBytes);
    gPropHost->propGetIntN(srcImg, kOfxImagePropBounds, 4, &srcRect.x1);
    gPropHost->propGetPointer(srcImg, kOfxImagePropData, 0, &srcPtr);
    gPropHost->propGetInt(dstImg, kOfxImagePropRowBytes, 0, &dstRowBytes);
    gPropHost->propGetIntN(dstImg, kOfxImagePropBounds, 4, &dstRect.x1);
    gPropHost->propGetPointer(dstImg, kOfxImagePropData, 0, &dstPtr);

    char* srcDepth = NULL;
    gPropHost->propGetString(srcImg, kOfxImageEffectPropPixelDepth, 0, &srcDepth);

    int bytesPerComponent = 4;
    if (srcDepth && strcmp(srcDepth, kOfxBitDepthByte) == 0) bytesPerComponent = 1;
    else if (srcDepth && strcmp(srcDepth, kOfxBitDepthShort) == 0) bytesPerComponent = 2;

    RenderArgs args;
    args.instance = instance;
    args.srcPtr = (char*)srcPtr;
    args.dstPtr = (char*)dstPtr;
    args.srcRect = srcRect;
    args.dstRect = dstRect;
    args.window = window;
    args.srcRowBytes = srcRowBytes;
    args.dstRowBytes = dstRowBytes;
    args.pixelBytes = bytesPerComponent * 4;
    args.strength = (float)strength;
    args.invSize = 1.0f / (float)(size < 1.0 ? 1.0 : size);
    args.seedX = seed + stepWrapped * 1013;
    args.seedY = seed + stepWrapped * 1013 + 1999;
    args.complexity = complexity < 1 ? 1 : (complexity > 6 ? 6 : complexity);
    args.noiseType = noiseType;
    args.qualityMode = qualityMode;

    if (args.strength <= 0.0001f) {
        for (int y = window.y1; y < window.y2; ++y) {
            char* dstRow = args.dstPtr + (ptrdiff_t)(y - args.dstRect.y1) * args.dstRowBytes;
            char* srcRow = args.srcPtr + (ptrdiff_t)(y - args.srcRect.y1) * args.srcRowBytes;
            std::memcpy(dstRow + (ptrdiff_t)(window.x1 - args.dstRect.x1) * args.pixelBytes,
                        srcRow + (ptrdiff_t)(window.x1 - args.srcRect.x1) * args.pixelBytes,
                        (size_t)(window.x2 - window.x1) * args.pixelBytes);
        }
        gEffectHost->clipReleaseImage(srcImg);
        gEffectHost->clipReleaseImage(dstImg);
        return kOfxStatOK;
    }

    if (gThreadHost) {
        unsigned int nCPUs = 1;
        gThreadHost->multiThreadNumCPUs(&nCPUs);
        if (nCPUs > 1) {
            gThreadHost->multiThread(renderSlice, nCPUs, &args);
        } else {
            renderSlice(0, 1, &args);
        }
    } else {
        renderSlice(0, 1, &args);
    }

    gEffectHost->clipReleaseImage(srcImg);
    gEffectHost->clipReleaseImage(dstImg);
    return kOfxStatOK;
}

static void setHostFunc(OfxHost* host) {
    gHost = host;
}

static OfxStatus pluginMain(const char* action, const void* handle, OfxPropertySetHandle inArgs, OfxPropertySetHandle outArgs) {
    OfxImageEffectHandle effect = (OfxImageEffectHandle)handle;
    if (strcmp(action, kOfxActionLoad) == 0) return onLoad();
    if (strcmp(action, kOfxActionDescribe) == 0) return describe(effect);
    if (strcmp(action, kOfxImageEffectActionDescribeInContext) == 0) return describeInContext(effect, inArgs);
    if (strcmp(action, kOfxActionCreateInstance) == 0) return createInstance(effect, inArgs);
    if (strcmp(action, kOfxActionDestroyInstance) == 0) return destroyInstance(effect, inArgs);
    if (strcmp(action, kOfxImageEffectActionRender) == 0) return render(effect, inArgs, outArgs);
    return kOfxStatReplyDefault;
}

static OfxPlugin plugin = {
    kOfxImageEffectPluginApi,
    1,
    "com.boilify.effect",
    1,
    6,
    setHostFunc,
    pluginMain
};

EXPORT OfxPlugin* OfxGetPlugin(int nth) {
    return (nth == 0) ? &plugin : NULL;
}

EXPORT int OfxGetNumberOfPlugins(void) {
    return 1;
}
