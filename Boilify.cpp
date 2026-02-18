#include <cstring>
#include <cmath>
#include <algorithm>
#include "ofxImageEffect.h"
#include "ofxMemory.h"
#include "ofxMultiThread.h"

#if defined __APPLE__ || defined __linux__ || defined __FreeBSD__
#define EXPORT __attribute__((visibility("default")))
#elif defined _WIN32
#define EXPORT OfxExport
#endif

#define PLUGIN_ID "com.boilify.effect"
#define PLUGIN_NAME "Boilify"
#define PLUGIN_VERSION_MAJOR 1
#define PLUGIN_VERSION_MINOR 0

#define PARAM_STRENGTH "strength"
#define PARAM_DENSITY "density"
#define PARAM_SPEED "speed"
#define PARAM_SEED "seed"
#define PARAM_ANIMATE "animate"

struct InstanceData {
    OfxImageClipHandle srcClip;
    OfxImageClipHandle dstClip;
    OfxParamHandle strengthParam;
    OfxParamHandle densityParam;
    OfxParamHandle speedParam;
    OfxParamHandle seedParam;
    OfxParamHandle animateParam;
};

static OfxHost* gHost;
static OfxImageEffectSuiteV1* gEffectHost = NULL;
static OfxPropertySuiteV1* gPropHost = NULL;
static OfxParameterSuiteV1* gParamHost = NULL;

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

static OfxStatus createInstance(OfxImageEffectHandle effect, OfxPropertySetHandle) {
    OfxPropertySetHandle effectProps;
    gEffectHost->getPropertySet(effect, &effectProps);
    
    OfxParamSetHandle paramSet;
    gEffectHost->getParamSet(effect, &paramSet);
    
    InstanceData* data = new InstanceData;
    
    gEffectHost->clipGetHandle(effect, kOfxImageEffectSimpleSourceClipName, &data->srcClip, NULL);
    gEffectHost->clipGetHandle(effect, kOfxImageEffectOutputClipName, &data->dstClip, NULL);
    
    gParamHost->paramGetHandle(paramSet, PARAM_STRENGTH, &data->strengthParam, NULL);
    gParamHost->paramGetHandle(paramSet, PARAM_DENSITY, &data->densityParam, NULL);
    gParamHost->paramGetHandle(paramSet, PARAM_SPEED, &data->speedParam, NULL);
    gParamHost->paramGetHandle(paramSet, PARAM_SEED, &data->seedParam, NULL);
    gParamHost->paramGetHandle(paramSet, PARAM_ANIMATE, &data->animateParam, NULL);
    
    gPropHost->propSetPointer(effectProps, kOfxPropInstanceData, 0, data);
    
    return kOfxStatOK;
}

static OfxStatus destroyInstance(OfxImageEffectHandle effect, OfxPropertySetHandle) {
    OfxPropertySetHandle effectProps;
    gEffectHost->getPropertySet(effect, &effectProps);
    
    InstanceData* data;
    gPropHost->propGetPointer(effectProps, kOfxPropInstanceData, 0, (void**)&data);
    delete data;
    
    return kOfxStatOK;
}

static OfxStatus describe(OfxImageEffectHandle effect) {
    OfxPropertySetHandle effectProps;
    gEffectHost->getPropertySet(effect, &effectProps);
    
    gPropHost->propSetString(effectProps, kOfxPropLabel, 0, PLUGIN_NAME);
    gPropHost->propSetString(effectProps, kOfxImageEffectPluginPropGrouping, 0, "Distort");
    
    gPropHost->propSetInt(effectProps, kOfxImageEffectPropSupportsMultipleClipDepths, 0, 0);
    // Keep this broadly compatible with hosts that prefer 8/16-bit or half-float pipelines.
    gPropHost->propSetString(effectProps, kOfxImageEffectPropSupportedPixelDepths, 0, kOfxBitDepthByte);
    gPropHost->propSetString(effectProps, kOfxImageEffectPropSupportedPixelDepths, 1, kOfxBitDepthShort);
    gPropHost->propSetString(effectProps, kOfxImageEffectPropSupportedPixelDepths, 2, kOfxBitDepthHalf);
    gPropHost->propSetString(effectProps, kOfxImageEffectPropSupportedPixelDepths, 3, kOfxBitDepthFloat);
    gPropHost->propSetString(effectProps, kOfxImageEffectPropSupportedContexts, 0, kOfxImageEffectContextFilter);
    
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
    
    gParamHost->paramDefine(params, kOfxParamTypeDouble, PARAM_STRENGTH, &props);
    gPropHost->propSetString(props, kOfxPropLabel, 0, "Strength");
    gPropHost->propSetDouble(props, kOfxParamPropDefault, 0, 1.0);
    gPropHost->propSetDouble(props, kOfxParamPropMin, 0, 0.0);
    gPropHost->propSetDouble(props, kOfxParamPropMax, 0, 20.0);
    gPropHost->propSetDouble(props, kOfxParamPropDisplayMin, 0, 0.0);
    gPropHost->propSetDouble(props, kOfxParamPropDisplayMax, 0, 5.0);
    
    gParamHost->paramDefine(params, kOfxParamTypeDouble, PARAM_DENSITY, &props);
    gPropHost->propSetString(props, kOfxPropLabel, 0, "Density");
    gPropHost->propSetDouble(props, kOfxParamPropDefault, 0, 2.0);
    gPropHost->propSetDouble(props, kOfxParamPropMin, 0, 0.1);
    gPropHost->propSetDouble(props, kOfxParamPropMax, 0, 20.0);
    gPropHost->propSetDouble(props, kOfxParamPropDisplayMin, 0, 0.1);
    gPropHost->propSetDouble(props, kOfxParamPropDisplayMax, 0, 10.0);
    
    gParamHost->paramDefine(params, kOfxParamTypeDouble, PARAM_SPEED, &props);
    gPropHost->propSetString(props, kOfxPropLabel, 0, "Speed");
    gPropHost->propSetDouble(props, kOfxParamPropDefault, 0, 1.0);
    gPropHost->propSetDouble(props, kOfxParamPropMin, 0, 0.0);
    gPropHost->propSetDouble(props, kOfxParamPropMax, 0, 10.0);
    gPropHost->propSetDouble(props, kOfxParamPropDisplayMin, 0, 0.0);
    gPropHost->propSetDouble(props, kOfxParamPropDisplayMax, 0, 5.0);
    
    gParamHost->paramDefine(params, kOfxParamTypeDouble, PARAM_SEED, &props);
    gPropHost->propSetString(props, kOfxPropLabel, 0, "Seed");
    gPropHost->propSetDouble(props, kOfxParamPropDefault, 0, 0.0);
    gPropHost->propSetDouble(props, kOfxParamPropMin, 0, 0.0);
    gPropHost->propSetDouble(props, kOfxParamPropMax, 0, 9999.0);
    
    gParamHost->paramDefine(params, kOfxParamTypeBoolean, PARAM_ANIMATE, &props);
    gPropHost->propSetString(props, kOfxPropLabel, 0, "Animate");
    gPropHost->propSetInt(props, kOfxParamPropDefault, 0, 1);
    
    return kOfxStatOK;
}

static OfxStatus render(OfxImageEffectHandle instance, OfxPropertySetHandle inArgs, OfxPropertySetHandle) {
    OfxPropertySetHandle effectProps;
    gEffectHost->getPropertySet(instance, &effectProps);
    
    InstanceData* data;
    gPropHost->propGetPointer(effectProps, kOfxPropInstanceData, 0, (void**)&data);
    
    double time;
    gPropHost->propGetDouble(inArgs, kOfxPropTime, 0, &time);
    
    double strength, density, speed, seed;
    int animate;
    gParamHost->paramGetValueAtTime(data->strengthParam, time, &strength);
    gParamHost->paramGetValueAtTime(data->densityParam, time, &density);
    gParamHost->paramGetValueAtTime(data->speedParam, time, &speed);
    gParamHost->paramGetValueAtTime(data->seedParam, time, &seed);
    gParamHost->paramGetValueAtTime(data->animateParam, time, &animate);
    
    float renderTime = animate ? (float)time : 0.0f;
    int seedInt = (int)seed;
    
    OfxRectI renderWindow;
    gPropHost->propGetIntN(inArgs, kOfxImageEffectPropRenderWindow, 4, &renderWindow.x1);
    
    OfxImageClipHandle srcClip = data->srcClip;
    OfxImageClipHandle dstClip = data->dstClip;
    
    OfxPropertySetHandle srcImg = NULL, dstImg = NULL;
    OfxStatus status = kOfxStatOK;
    
    if(gEffectHost->clipGetImage(dstClip, time, NULL, &dstImg) != kOfxStatOK) return kOfxStatFailed;
    if(gEffectHost->clipGetImage(srcClip, time, NULL, &srcImg) != kOfxStatOK) {
        gEffectHost->clipReleaseImage(dstImg);
        return kOfxStatFailed;
    }
    
    int srcRowBytes, dstRowBytes;
    OfxRectI srcRect, dstRect;
    void *srcPtr, *dstPtr;
    
    gPropHost->propGetInt(srcImg, kOfxImagePropRowBytes, 0, &srcRowBytes);
    gPropHost->propGetIntN(srcImg, kOfxImagePropBounds, 4, &srcRect.x1);
    gPropHost->propGetPointer(srcImg, kOfxImagePropData, 0, &srcPtr);
    
    gPropHost->propGetInt(dstImg, kOfxImagePropRowBytes, 0, &dstRowBytes);
    gPropHost->propGetIntN(dstImg, kOfxImagePropBounds, 4, &dstRect.x1);
    gPropHost->propGetPointer(dstImg, kOfxImagePropData, 0, &dstPtr);
    
    const char* srcDepth = NULL;
    const char* dstDepth = NULL;
    gPropHost->propGetString(srcImg, kOfxImageEffectPropPixelDepth, 0, &srcDepth);
    gPropHost->propGetString(dstImg, kOfxImageEffectPropPixelDepth, 0, &dstDepth);

    // We only do nearest-neighbor pixel moves, so we can memcpy the pixel bytes.
    int bytesPerComponent = 0;
    if(srcDepth && strcmp(srcDepth, kOfxBitDepthByte) == 0) bytesPerComponent = 1;
    else if(srcDepth && strcmp(srcDepth, kOfxBitDepthShort) == 0) bytesPerComponent = 2;
    else if(srcDepth && strcmp(srcDepth, kOfxBitDepthHalf) == 0) bytesPerComponent = 2;
    else if(srcDepth && strcmp(srcDepth, kOfxBitDepthFloat) == 0) bytesPerComponent = 4;
    else {
        gEffectHost->clipReleaseImage(srcImg);
        gEffectHost->clipReleaseImage(dstImg);
        return kOfxStatErrUnsupported;
    }

    if(!dstDepth || strcmp(srcDepth, dstDepth) != 0) {
        gEffectHost->clipReleaseImage(srcImg);
        gEffectHost->clipReleaseImage(dstImg);
        return kOfxStatErrUnsupported;
    }

    const int pixelBytes = bytesPerComponent * 4; // RGBA
    
    float scale = (float)(density / 100.0);
    float t = renderTime * (float)speed;
    
    for (int y = renderWindow.y1; y < renderWindow.y2; y++) {
        if(gEffectHost->abort(instance)) break;
        
        for (int x = renderWindow.x1; x < renderWindow.x2; x++) {
            int px = x - dstRect.x1;
            int py = y - dstRect.y1;
            
            float n = perlin(px * scale + t, py * scale + t, seedInt);
            float ox = cosf(n * 6.28318f) * (float)strength;
            float oy = sinf(n * 6.28318f) * (float)strength;

            const int sxAbs = std::max(srcRect.x1, std::min(srcRect.x2 - 1, x + (int)ox));
            const int syAbs = std::max(srcRect.y1, std::min(srcRect.y2 - 1, y + (int)oy));

            const int srcX = sxAbs - srcRect.x1;
            const int srcY = syAbs - srcRect.y1;
            const int dstX = x - dstRect.x1;
            const int dstY = y - dstRect.y1;

            char* dstRow = (char*)dstPtr + (ptrdiff_t)dstY * (ptrdiff_t)dstRowBytes;
            char* srcRow = (char*)srcPtr + (ptrdiff_t)srcY * (ptrdiff_t)srcRowBytes;

            char* dstPixel = dstRow + (ptrdiff_t)dstX * (ptrdiff_t)pixelBytes;
            char* srcPixel = srcRow + (ptrdiff_t)srcX * (ptrdiff_t)pixelBytes;

            memcpy(dstPixel, srcPixel, (size_t)pixelBytes);
        }
    }
    
    if(srcImg) gEffectHost->clipReleaseImage(srcImg);
    if(dstImg) gEffectHost->clipReleaseImage(dstImg);
    
    return status;
}

static OfxStatus onLoad(void) {
    if(!gHost) return kOfxStatErrMissingHostFeature;
    
    gEffectHost = (OfxImageEffectSuiteV1*)gHost->fetchSuite(gHost->host, kOfxImageEffectSuite, 1);
    gPropHost = (OfxPropertySuiteV1*)gHost->fetchSuite(gHost->host, kOfxPropertySuite, 1);
    gParamHost = (OfxParameterSuiteV1*)gHost->fetchSuite(gHost->host, kOfxParameterSuite, 1);
    
    if(!gEffectHost || !gPropHost || !gParamHost)
        return kOfxStatErrMissingHostFeature;
    return kOfxStatOK;
}

static OfxStatus pluginMain(const char* action, const void* handle, OfxPropertySetHandle inArgs, OfxPropertySetHandle outArgs) {
    try {
        OfxImageEffectHandle effect = (OfxImageEffectHandle)handle;
        
        if(strcmp(action, kOfxActionLoad) == 0) {
            return onLoad();
        }
        else if(strcmp(action, kOfxActionCreateInstance) == 0) {
            return createInstance(effect, inArgs);
        }
        else if(strcmp(action, kOfxActionDestroyInstance) == 0) {
            return destroyInstance(effect, inArgs);
        }
        else if(strcmp(action, kOfxActionDescribe) == 0) {
            return describe(effect);
        }
        else if(strcmp(action, kOfxImageEffectActionDescribeInContext) == 0) {
            return describeInContext(effect, inArgs);
        }
        else if(strcmp(action, kOfxImageEffectActionRender) == 0) {
            return render(effect, inArgs, outArgs);
        }
    } catch(...) {
        return kOfxStatErrUnknown;
    }
    
    return kOfxStatReplyDefault;
}

static void setHostFunc(OfxHost* hostStruct) {
    gHost = hostStruct;
}

static OfxPlugin boilifyPlugin = {       
    kOfxImageEffectPluginApi,
    1,
    PLUGIN_ID,
    PLUGIN_VERSION_MAJOR,
    PLUGIN_VERSION_MINOR,
    setHostFunc,
    pluginMain
};

EXPORT OfxPlugin* OfxGetPlugin(int nth) {
    if(nth == 0) return &boilifyPlugin;
    return 0;
}

EXPORT int OfxGetNumberOfPlugins(void) {
    return 1;
}
