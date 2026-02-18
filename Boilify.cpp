#include <cstring>
#include <cmath>
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

static OfxStatus onLoad(void) {
    if (!gHost) return kOfxStatErrMissingHostFeature;
    gEffectHost = (OfxImageEffectSuiteV1*)gHost->fetchSuite(gHost->host, kOfxImageEffectSuite, 1);
    gPropHost = (OfxPropertySuiteV1*)gHost->fetchSuite(gHost->host, kOfxPropertySuite, 1);
    gParamHost = (OfxParameterSuiteV1*)gHost->fetchSuite(gHost->host, kOfxParameterSuite, 1);
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

    return kOfxStatOK;
}

static OfxStatus createInstance(OfxImageEffectHandle effect, OfxPropertySetHandle) {
    return kOfxStatOK;
}

static OfxStatus destroyInstance(OfxImageEffectHandle effect, OfxPropertySetHandle) {
    return kOfxStatOK;
}

static float hash(float x, float y, int seed) {
    float n = sinf(x * 12.9898f + y * 78.233f + seed * 43.758f) * 43758.5453f;
    return n - floorf(n);
}

static float perlin(float x, float y, int seed) {
    float x0 = floorf(x);
    float y0 = floorf(y);
    float fx = x - x0;
    float fy = y - y0;
    float sx = fx * fx * (3.0f - 2.0f * fx);
    float sy = fy * fy * (3.0f - 2.0f * fy);
    float n00 = hash(x0, y0, seed);
    float n10 = hash(x0 + 1, y0, seed);
    float n01 = hash(x0, y0 + 1, seed);
    float n11 = hash(x0 + 1, y0 + 1, seed);
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

    OfxParamHandle strengthParam, densityParam, speedParam, seedParam, animateParam;
    gParamHost->paramGetHandle(paramSet, "strength", &strengthParam, NULL);
    gParamHost->paramGetHandle(paramSet, "density", &densityParam, NULL);
    gParamHost->paramGetHandle(paramSet, "speed", &speedParam, NULL);
    gParamHost->paramGetHandle(paramSet, "seed", &seedParam, NULL);
    gParamHost->paramGetHandle(paramSet, "animate", &animateParam, NULL);

    double strength, density, speed, seed;
    int animate;
    gParamHost->paramGetValue(strengthParam, &strength);
    gParamHost->paramGetValue(densityParam, &density);
    gParamHost->paramGetValue(speedParam, &speed);
    gParamHost->paramGetValue(seedParam, &seed);
    gParamHost->paramGetValue(animateParam, &animate);

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

    float scale = (float)(density / 100.0);
    float t = renderTime * (float)speed;

    for (int y = window.y1; y < window.y2; y++) {
        if (gEffectHost->abort(instance)) break;
        for (int x = window.x1; x < window.x2; x++) {
            int px = x - dstRect.x1;
            int py = y - dstRect.y1;

            float n = perlin(px * scale + t, py * scale + t, seedInt);
            float ox = cosf(n * 6.28318f) * (float)strength;
            float oy = sinf(n * 6.28318f) * (float)strength;

            int srcX = x + (int)ox;
            int srcY = y + (int)oy;
            srcX = srcX < srcRect.x1 ? srcRect.x1 : (srcX >= srcRect.x2 ? srcRect.x2 - 1 : srcX);
            srcY = srcY < srcRect.y1 ? srcRect.y1 : (srcY >= srcRect.y2 ? srcRect.y2 - 1 : srcY);

            char* dstRow = (char*)dstPtr + (ptrdiff_t)(y - dstRect.y1) * dstRowBytes;
            char* srcRow = (char*)srcPtr + (ptrdiff_t)(srcY - srcRect.y1) * srcRowBytes;
            memcpy(dstRow + (ptrdiff_t)(x - dstRect.x1) * pixelBytes,
                   srcRow + (ptrdiff_t)(srcX - srcRect.x1) * pixelBytes,
                   (size_t)pixelBytes);
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
