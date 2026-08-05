#define HL_NAME(n)  dlss_##n
#include <hl.h>
#undef _GUID

#include <vector>

#include <sl.h>
#include <sl_consts.h>
#include <sl_dlss.h>
#include <sl_hooks.h>
#include <sl_security.h>

#ifdef HL_WIN_DESKTOP
#include <filesystem>
#include <dxgi.h>
#include <dxgi1_5.h>
#include <d3d12.h>
#include <dxcapi.h>
#endif

#define _DEVICE _ABSTRACT(dx_device)
#define _FACTORY _ABSTRACT(dx_factory)
#define _ADAPTER _ABSTRACT(dx_adapter)
#define _RES _ABSTRACT(dx_resource)

namespace slFuncs {
SL_FUN_DECL(slInit);
SL_FUN_DECL(slShutdown);
SL_FUN_DECL(slIsFeatureSupported);
SL_FUN_DECL(slIsFeatureLoaded);
SL_FUN_DECL(slSetFeatureLoaded);
SL_FUN_DECL(slEvaluateFeature);
SL_FUN_DECL(slAllocateResources);
SL_FUN_DECL(slFreeResources);
SL_FUN_DECL(slSetTagForFrame);
SL_FUN_DECL(slGetFeatureRequirements);
SL_FUN_DECL(slGetFeatureVersion);
SL_FUN_DECL(slUpgradeInterface);
SL_FUN_DECL(slSetConstants);
SL_FUN_DECL(slGetNativeInterface);
SL_FUN_DECL(slGetFeatureFunction);
SL_FUN_DECL(slGetNewFrameToken);
SL_FUN_DECL(slSetD3DDevice);
static PFun_slDLSSGetOptimalSettings* slDLSSGetOptimalSettings{};
static PFun_slDLSSSetOptions* slDLSSSetOptions{};
}

#define LOAD_SL_FUNC(name) \
slFuncs::name = reinterpret_cast<PFun_##name*>(GetProcAddress(mod, #name))

enum DLSSFeature {
    DLSS,
    FrameGen
};

sl::Feature toSlFeature(DLSSFeature feature) {
    sl::Feature featureId = 0;
    switch (feature) {
        case DLSSFeature::DLSS: {
            featureId = sl::kFeatureDLSS;
            break;
        }
        case DLSSFeature::FrameGen: {
            featureId = sl::kFeatureDLSS_G;
            break;
        }
    }
    return featureId;
}

HL_PRIM int HL_NAME(init)(bool showConsole) {

    wchar_t path[2048] = { 0 };

    DWORD len = GetModuleFileNameW(nullptr, path, MAX_PATH);
    if (len == 0)
        return -1;

    std::filesystem::path basePath = std::filesystem::path(path).parent_path();
    std::filesystem::path dllPath = basePath / L"sl.interposer.dll";
    if (!sl::security::verifyEmbeddedSignature(dllPath.c_str()))
        return -1;

    HMODULE mod = LoadLibraryW(dllPath.c_str());

    LOAD_SL_FUNC(slInit);
    LOAD_SL_FUNC(slShutdown);
    LOAD_SL_FUNC(slIsFeatureSupported);
    LOAD_SL_FUNC(slIsFeatureLoaded);
    LOAD_SL_FUNC(slSetFeatureLoaded);
    LOAD_SL_FUNC(slEvaluateFeature);
    LOAD_SL_FUNC(slAllocateResources);
    LOAD_SL_FUNC(slFreeResources);
    LOAD_SL_FUNC(slSetTagForFrame);
    LOAD_SL_FUNC(slGetFeatureRequirements);
    LOAD_SL_FUNC(slGetFeatureVersion);
    LOAD_SL_FUNC(slUpgradeInterface);
    LOAD_SL_FUNC(slSetConstants);
    LOAD_SL_FUNC(slGetNativeInterface);
    LOAD_SL_FUNC(slGetFeatureFunction);
    LOAD_SL_FUNC(slGetNewFrameToken);
    LOAD_SL_FUNC(slSetD3DDevice);

    sl::Preferences pref{};
    pref.showConsole = showConsole;
    pref.logLevel = sl::LogLevel::eOff;
    pref.engine = sl::EngineType::eCustom;
    pref.projectId = "5346cce9-f379-43da-b490-74f1194b1e8f";
    pref.engineVersion = "2.1.1";
    sl::Feature featureList[] = { sl::kFeatureDLSS /*, sl::kFeatureDLSS_G*/ };
    pref.featuresToLoad = featureList;
    pref.numFeaturesToLoad = _countof(featureList);
    pref.flags |= sl::PreferenceFlags::eUseFrameBasedResourceTagging | sl::PreferenceFlags::eUseManualHooking;

    sl::Result res = slFuncs::slInit(pref, sl::kSDKVersion);
    return static_cast<int>(res);
}

HL_PRIM int HL_NAME(shutdown)() {
    sl::Result res = slFuncs::slShutdown();
    return static_cast<int>(res);
}

HL_PRIM int HL_NAME(set_device)(void* nativeDevice) {
    sl::Result res = slFuncs::slSetD3DDevice(nativeDevice);
    if (res != sl::Result::eOk) 
        return static_cast<int>(res);

    slFuncs::slGetFeatureFunction(sl::kFeatureDLSS, "slDLSSGetOptimalSettings", (void*&)slFuncs::slDLSSGetOptimalSettings);
    slFuncs::slGetFeatureFunction(sl::kFeatureDLSS, "slDLSSSetOptions", (void*&)slFuncs::slDLSSSetOptions);

    return static_cast<int>(res);
}

void* upgradeInterface(void* dx_interface) {
    IUnknown* base = (IUnknown*)dx_interface;
    sl::Result res = slFuncs::slUpgradeInterface(&dx_interface);
    if (res == sl::Result::eOk && dx_interface != base)
        base->Release();
    return dx_interface;
}

HL_PRIM void* HL_NAME(upgrade_device)(void* nativeDevice) {
    return upgradeInterface(nativeDevice);
}

HL_PRIM void* HL_NAME(upgrade_factory)(void* nativeFactory) {
    return upgradeInterface(nativeFactory);
}

HL_PRIM int HL_NAME(is_feature_supported)(IDXGIAdapter* adapter, DLSSFeature feature) {
    DXGI_ADAPTER_DESC desc;
    adapter->GetDesc(&desc);
    sl::AdapterInfo adapterInfo;
    adapterInfo.deviceLUID = (uint8_t*)&desc.AdapterLuid;
    adapterInfo.deviceLUIDSizeInBytes = sizeof(LUID);

    sl::Result res = slFuncs::slIsFeatureSupported(toSlFeature(feature), adapterInfo);
    return static_cast<int>(res);
}

struct DLSSOptions {
    sl::DLSSMode mode;
    uint32_t outputWidth;
    uint32_t outputHeight;
    sl::DLSSPreset preset;
    int colorBufferHDR;
    int autoExposure;
};

struct DLSSOptimalSettings {
    uint32_t optimalRenderWidth;
    uint32_t optimalRenderHeight;
    double optimalSharpness;
};

HL_PRIM int HL_NAME(get_optimal_settings)(DLSSOptions* options, DLSSOptimalSettings* outOptimalSettings) {
    sl::DLSSOptions dlssOptions;
    dlssOptions.mode = options->mode;
    dlssOptions.outputWidth = options->outputWidth;
    dlssOptions.outputHeight = options->outputHeight;

    sl::DLSSOptimalSettings optimalSettings;
    sl::Result res = slFuncs::slDLSSGetOptimalSettings(dlssOptions, optimalSettings);

    outOptimalSettings->optimalRenderWidth = optimalSettings.optimalRenderWidth;
    outOptimalSettings->optimalRenderHeight = optimalSettings.optimalRenderHeight;
    outOptimalSettings->optimalSharpness = (double)optimalSettings.optimalSharpness;

    return static_cast<int>(res);
}

typedef sl::FrameToken dlss_frametoken;

#define _FRAMETOKEN _ABSTRACT(dlss_frametoken)

HL_PRIM sl::FrameToken* HL_NAME(get_new_frame_token)(int frameIndex) {
    sl::FrameToken* frameToken = nullptr;
    uint32_t frameId = (uint32_t)frameIndex;
    slFuncs::slGetNewFrameToken(frameToken, &frameId);
    return frameToken;
}

enum DLSSBufferType {
    Depth,
    MotionVectors,
    ColorIn,
    ColorOut
};

struct DLSSResource {
    ID3D12Resource* res;
    int width;
    int height;
    DLSSBufferType type;
    D3D12_RESOURCE_STATES state;
};

HL_PRIM int HL_NAME(set_tag_for_frame)(sl::FrameToken* frameToken, DLSSResource* res, int count, ID3D12GraphicsCommandList* cmdList) {
    std::vector<sl::Resource> slResources(count);
    std::vector<sl::Extent> slExtents(count);
    std::vector<sl::ResourceTag> slTags(count);

    for (int i = 0; i < count; i++) {
        DLSSResource& r = res[i];

        slResources[i] = { sl::ResourceType::eTex2d, r.res, (uint32_t)r.state };
        slExtents[i] = { 0, 0, (uint32_t)r.width, (uint32_t)r.height };

        sl::BufferType type = {};
        switch (r.type) {
        case DLSSBufferType::Depth: type = sl::kBufferTypeDepth; break;
        case DLSSBufferType::MotionVectors: type = sl::kBufferTypeMotionVectors; break;
        case DLSSBufferType::ColorIn: type = sl::kBufferTypeScalingInputColor; break;
        case DLSSBufferType::ColorOut: type = sl::kBufferTypeScalingOutputColor; break;
        }

        slTags[i] = { &slResources[i], type, sl::ResourceLifecycle::eValidUntilPresent, &slExtents[i] };
    }

    sl::Result result = slFuncs::slSetTagForFrame(*frameToken, sl::ViewportHandle(0), slTags.data(), (uint32_t)count, cmdList);

    return static_cast<int>(result);
}

HL_PRIM int HL_NAME(set_options)(DLSSOptions* options) {
    sl::DLSSOptions dlssOptions;
    dlssOptions.mode = options->mode;
    dlssOptions.outputWidth = options->outputWidth;
    dlssOptions.outputHeight = options->outputHeight;
    dlssOptions.dlaaPreset = options->preset;
    dlssOptions.colorBuffersHDR = options->colorBufferHDR ? sl::Boolean::eTrue : sl::Boolean::eFalse;
    dlssOptions.useAutoExposure = options->autoExposure ? sl::Boolean::eTrue : sl::Boolean::eFalse;

    sl::Result result = slFuncs::slDLSSSetOptions(sl::ViewportHandle(0), dlssOptions);

    return static_cast<int>(result);
}

struct DLSSConstants {
    float* cameraViewToClip;
    float* clipToCameraView;
    float* clipToLensClip;
    float* clipToPrevClip;
    float* prevClipToClip;
    float jitterOffsetX;
    float jitterOffsetY;
    float mvecScaleX;
    float mvecScaleY;
    float cameraPinholeOffsetX;
    float cameraPinholeOffsetY;
    float* cameraPos;
    float* cameraUp;
    float* cameraRight;
    float* cameraFwd;
    float cameraNear;
    float cameraFar;
    float cameraFOV;
    float cameraAspectRatio;
    float motionVectorsInvalidValue;
    int depthInverted;
    int cameraMotionIncluded;
    int motionVectors3D;
    int reset;
    int orthographicProjection;
    int motionVectorsDilated;
    int motionVectorsJittered;
    float minRelativeLinearDepthObjectSeparation;
};

HL_PRIM int HL_NAME(set_constants)(sl::FrameToken* frameToken, DLSSConstants* constants) {
    sl::Constants slConstants{};
    memcpy(&slConstants.cameraViewToClip, constants->cameraViewToClip, sizeof(float) * 16);
    memcpy(&slConstants.clipToCameraView, constants->clipToCameraView, sizeof(float) * 16);
    memcpy(&slConstants.clipToLensClip, constants->clipToLensClip, sizeof(float) * 16);
    memcpy(&slConstants.clipToPrevClip, constants->clipToPrevClip, sizeof(float) * 16);
    memcpy(&slConstants.prevClipToClip, constants->prevClipToClip, sizeof(float) * 16);
    slConstants.jitterOffset = { constants->jitterOffsetX, constants->jitterOffsetY };
    slConstants.mvecScale = { constants->mvecScaleX,    constants->mvecScaleY };
    slConstants.cameraPinholeOffset = { constants->cameraPinholeOffsetX, constants->cameraPinholeOffsetY };
    memcpy(&slConstants.cameraPos, constants->cameraPos, sizeof(float) * 3);
    memcpy(&slConstants.cameraUp, constants->cameraUp, sizeof(float) * 3);
    memcpy(&slConstants.cameraRight, constants->cameraRight, sizeof(float) * 3);
    memcpy(&slConstants.cameraFwd, constants->cameraFwd, sizeof(float) * 3);
    slConstants.cameraNear = constants->cameraNear;
    slConstants.cameraFar = constants->cameraFar;
    slConstants.cameraFOV = constants->cameraFOV;
    slConstants.cameraAspectRatio = constants->cameraAspectRatio;
    slConstants.motionVectorsInvalidValue = constants->motionVectorsInvalidValue;
    slConstants.depthInverted = constants->depthInverted ? sl::Boolean::eTrue : sl::Boolean::eFalse;
    slConstants.cameraMotionIncluded = constants->cameraMotionIncluded ? sl::Boolean::eTrue : sl::Boolean::eFalse;
    slConstants.motionVectors3D = constants->motionVectors3D ? sl::Boolean::eTrue : sl::Boolean::eFalse;
    slConstants.reset = constants->reset ? sl::Boolean::eTrue : sl::Boolean::eFalse;
    slConstants.orthographicProjection = constants->orthographicProjection ? sl::Boolean::eTrue : sl::Boolean::eFalse;
    slConstants.motionVectorsDilated = constants->motionVectorsDilated ? sl::Boolean::eTrue : sl::Boolean::eFalse;
    slConstants.motionVectorsJittered = constants->motionVectorsJittered ? sl::Boolean::eTrue : sl::Boolean::eFalse;
    slConstants.minRelativeLinearDepthObjectSeparation = constants->minRelativeLinearDepthObjectSeparation;

    sl::Result result = slFuncs::slSetConstants(slConstants, *frameToken, sl::ViewportHandle(0));
    return static_cast<int>(result);
}

HL_PRIM int HL_NAME(evaluate_feature)(sl::FrameToken* frameToken, ID3D12GraphicsCommandList* cmdList, DLSSFeature feature) {
    sl::ViewportHandle vp = { sl::ViewportHandle(0) };
    const sl::BaseStructure* inputs[] = { &vp };

    sl::Result result = slFuncs::slEvaluateFeature(toSlFeature(feature), *frameToken, inputs, _countof(inputs), cmdList);
    return static_cast<int>(result);
}

DEFINE_PRIM(_I32, init, _BOOL);
DEFINE_PRIM(_I32, shutdown, _NO_ARG);
DEFINE_PRIM(_DEVICE, upgrade_device, _DEVICE);
DEFINE_PRIM(_FACTORY, upgrade_factory, _FACTORY);
DEFINE_PRIM(_I32, set_device, _DEVICE);
DEFINE_PRIM(_I32, is_feature_supported, _ADAPTER _I32);
DEFINE_PRIM(_I32, get_optimal_settings, _STRUCT _STRUCT);
DEFINE_PRIM(_FRAMETOKEN, get_new_frame_token, _I32);
DEFINE_PRIM(_I32, set_tag_for_frame, _FRAMETOKEN _ABSTRACT(hl_carray) _I32 _RES);
DEFINE_PRIM(_I32, set_options, _STRUCT);
DEFINE_PRIM(_I32, set_constants, _FRAMETOKEN _STRUCT);
DEFINE_PRIM(_I32, evaluate_feature, _FRAMETOKEN _RES _I32);