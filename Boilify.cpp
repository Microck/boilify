#include <cstring>
#include <cmath>
#include <cstdint>
#include "ofxImageEffect.h"

#if defined _WIN32
  #define EXPORT OfxExport
#else
  #define EXPORT __attribute__((visibility("default")))
#endif

static OfxHost* gHost = NULL;
static OfxImageEffectSuiteV1* gEffectHost = NULL;
static OfxPropertySuiteV1* gPropHost = NULL;
static OfxParameterSuiteV1* gParamHost = NULL;

static constexpr float kTau = 6.28318530718f;
static constexpr int kAngleLutSize = 1024;
static constexpr int kAngleLutMask = kAngleLutSize - 1;
static constexpr int kQualityFast = 0;
static constexpr int kQualityHigh = 1;
static float gCosLut[kAngleLutSize];
static float gSinLut[kAngleLutSize];
static bool gAngleLutReady = false;

static_assert((kAngleLutSize & (kAngleLutSize - 1)) == 0, "kAngleLutSize must be a power of two");

static void initAngleLut(void) {
    if (gAngleLutReady) return;
    for (int i = 0; i < kAngleLutSize; ++i) {
        float angle = (float)i * (kTau / (float)kAngleLutSize);
        gCosLut[i] = cosf(angle);
        gSinLut[i] = sinf(angle);
    }
    gAngleLutReady = true;
}

static OfxStatus onLoad(void) {
    if (!gHost) return kOfxStatErrMissingHostFeature;
    gEffectHost = (OfxImageEffectSuiteV1*)gHost->fetchSuite(gHost->host, kOfxImageEffectSuite, 1);
    gPropHost = (OfxPropertySuiteV1*)gHost->fetchSuite(gHost->host, kOfxPropertySuite, 1);
    gParamHost = (OfxParameterSuiteV1*)gHost->fetchSuite(gHost->host, kOfxParameterSuite, 1);
    if (!gEffectHost || !gPropHost || !gParamHost) return kOfxStatErrMissingHostFeature;
    initAngleLut();
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
    gPropHost->propSetDouble(props, kOfxParamPropDefault, 0, 1.0);
    gPropHost->propSetDouble(props, kOfxParamPropMin, 0, 0.0);
    gPropHost->propSetDouble(props, kOfxParamPropMax, 0, 20.0);
    gPropHost->propSetDouble(props, kOfxParamPropDisplayMin, 0, 0.0);
    gPropHost->propSetDouble(props, kOfxParamPropDisplayMax, 0, 5.0);

    gParamHost->paramDefine(params, kOfxParamTypeDouble, "density", &props);
    gPropHost->propSetString(props, kOfxPropLabel, 0, "Density");
    gPropHost->propSetDouble(props, kOfxParamPropDefault, 0, 2.0);
    gPropHost->propSetDouble(props, kOfxParamPropMin, 0, 0.1);
    gPropHost->propSetDouble(props, kOfxParamPropMax, 0, 20.0);

    gParamHost->paramDefine(params, kOfxParamTypeDouble, "speed", &props);
    gPropHost->propSetString(props, kOfxPropLabel, 0, "Speed");
    gPropHost->propSetDouble(props, kOfxParamPropDefault, 0, 1.0);
    gPropHost->propSetDouble(props, kOfxParamPropMin, 0, 0.0);
    gPropHost->propSetDouble(props, kOfxParamPropMax, 0, 10.0);

    gParamHost->paramDefine(params, kOfxParamTypeDouble, "seed", &props);
    gPropHost->propSetString(props, kOfxPropLabel, 0, "Seed");
    gPropHost->propSetDouble(props, kOfxParamPropDefault, 0, 0.0);
    gPropHost->propSetDouble(props, kOfxParamPropMin, 0, 0.0);
    gPropHost->propSetDouble(props, kOfxParamPropMax, 0, 9999.0);

    gParamHost->paramDefine(params, kOfxParamTypeBoolean, "animate", &props);
    gPropHost->propSetString(props, kOfxPropLabel, 0, "Animate");
    gPropHost->propSetInt(props, kOfxParamPropDefault, 0, 1);

    gParamHost->paramDefine(params, kOfxParamTypeChoice, "quality", &props);
    gPropHost->propSetString(props, kOfxPropLabel, 0, "Quality");
    gPropHost->propSetInt(props, kOfxParamPropDefault, 0, kQualityFast);
    gPropHost->propSetString(props, kOfxParamPropChoiceOption, 0, "Fast");
    gPropHost->propSetString(props, kOfxParamPropChoiceOption, 1, "High");
    gPropHost->propSetString(props, kOfxParamPropHint, 0, "Fast improves preview speed. High gives smoother distortion.");

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

static inline float hashHigh(float x, float y, int seed) {
    float n = sinf(x * 12.9898f + y * 78.233f + seed * 43.758f) * 43758.5453f;
    return n - floorf(n);
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
    float nx0 = n00 + sx * (n10 - n00);
    float nx1 = n01 + sx * (n11 - n01);
    return nx0 + sy * (nx1 - nx0);
}

static float perlinHigh(float x, float y, int seed) {
    float x0 = floorf(x);
    float y0 = floorf(y);
    float fx = x - x0;
    float fy = y - y0;
    float sx = fx * fx * (3.0f - 2.0f * fx);
    float sy = fy * fy * (3.0f - 2.0f * fy);
    float n00 = hashHigh(x0, y0, seed);
    float n10 = hashHigh(x0 + 1.0f, y0, seed);
    float n01 = hashHigh(x0, y0 + 1.0f, seed);
    float n11 = hashHigh(x0 + 1.0f, y0 + 1.0f, seed);
    float nx0 = n00 + sx * (n10 - n00);
    float nx1 = n01 + sx * (n11 - n01);
    return nx0 + sy * (nx1 - nx0);
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

    OfxParamHandle strengthParam, densityParam, speedParam, seedParam, animateParam, qualityParam;
    gParamHost->paramGetHandle(paramSet, "strength", &strengthParam, NULL);
    gParamHost->paramGetHandle(paramSet, "density", &densityParam, NULL);
    gParamHost->paramGetHandle(paramSet, "speed", &speedParam, NULL);
    gParamHost->paramGetHandle(paramSet, "seed", &seedParam, NULL);
    gParamHost->paramGetHandle(paramSet, "animate", &animateParam, NULL);
    gParamHost->paramGetHandle(paramSet, "quality", &qualityParam, NULL);

    double strength, density, speed, seed;
    int animate, qualityMode;
    gParamHost->paramGetValue(strengthParam, &strength);
    gParamHost->paramGetValue(densityParam, &density);
    gParamHost->paramGetValue(speedParam, &speed);
    gParamHost->paramGetValue(seedParam, &seed);
    gParamHost->paramGetValue(animateParam, &animate);
    gParamHost->paramGetValue(qualityParam, &qualityMode);

    float renderTime = animate ? (float)time : 0.0f;
    int seedInt = (int)seed;

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
    const int pixelBytes = bytesPerComponent * 4;

    const float scale = (float)(density * 0.01);
    const float t = renderTime * (float)speed;
    const float strengthF = (float)strength;
    const bool highQuality = (qualityMode == kQualityHigh);

    if (highQuality) {
        for (int y = window.y1; y < window.y2; y++) {
            if (gEffectHost->abort(instance)) break;
            char* dstRow = (char*)dstPtr + (ptrdiff_t)(y - dstRect.y1) * dstRowBytes;
            const float noiseY = (float)(y - dstRect.y1) * scale + t;
            for (int x = window.x1; x < window.x2; x++) {
                const int dstX = x - dstRect.x1;
                const float noiseX = (float)dstX * scale + t;
                const float n = perlinHigh(noiseX, noiseY, seedInt);
                const float angle = n * kTau;

                int srcX = x + (int)(cosf(angle) * strengthF);
                int srcY = y + (int)(sinf(angle) * strengthF);
                srcX = srcX < srcRect.x1 ? srcRect.x1 : (srcX >= srcRect.x2 ? srcRect.x2 - 1 : srcX);
                srcY = srcY < srcRect.y1 ? srcRect.y1 : (srcY >= srcRect.y2 ? srcRect.y2 - 1 : srcY);

                char* srcRow = (char*)srcPtr + (ptrdiff_t)(srcY - srcRect.y1) * srcRowBytes;
                std::memcpy(dstRow + (ptrdiff_t)dstX * pixelBytes,
                            srcRow + (ptrdiff_t)(srcX - srcRect.x1) * pixelBytes,
                            (size_t)pixelBytes);
            }
        }
    } else {
        for (int y = window.y1; y < window.y2; y++) {
            if (gEffectHost->abort(instance)) break;
            char* dstRow = (char*)dstPtr + (ptrdiff_t)(y - dstRect.y1) * dstRowBytes;
            const float noiseY = (float)(y - dstRect.y1) * scale + t;
            for (int x = window.x1; x < window.x2; x++) {
                const int dstX = x - dstRect.x1;
                const float noiseX = (float)dstX * scale + t;
                const float n = perlinFast(noiseX, noiseY, seedInt);
                const int angleIndex = (int)(n * (float)kAngleLutSize) & kAngleLutMask;

                int srcX = x + (int)(gCosLut[angleIndex] * strengthF);
                int srcY = y + (int)(gSinLut[angleIndex] * strengthF);
                srcX = srcX < srcRect.x1 ? srcRect.x1 : (srcX >= srcRect.x2 ? srcRect.x2 - 1 : srcX);
                srcY = srcY < srcRect.y1 ? srcRect.y1 : (srcY >= srcRect.y2 ? srcRect.y2 - 1 : srcY);

                char* srcRow = (char*)srcPtr + (ptrdiff_t)(srcY - srcRect.y1) * srcRowBytes;
                std::memcpy(dstRow + (ptrdiff_t)dstX * pixelBytes,
                            srcRow + (ptrdiff_t)(srcX - srcRect.x1) * pixelBytes,
                            (size_t)pixelBytes);
            }
        }
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
    0,
    setHostFunc,
    pluginMain
};

EXPORT OfxPlugin* OfxGetPlugin(int nth) {
    return (nth == 0) ? &plugin : NULL;
}

EXPORT int OfxGetNumberOfPlugins(void) {
    return 1;
}
