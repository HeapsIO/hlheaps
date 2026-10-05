#define HL_NAME(n)  fsr_##n
#include <hl.h>
#undef _GUID

#include <windows.h>
#include <d3d12.h>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <string>

#include <ffx_api.h>
#include <ffx_api_types.h>
#include <dx12/ffx_api_dx12.h>
#include <ffx_upscale.h>

#define _DEVICE _ABSTRACT(dx_device)
#define _RES _ABSTRACT(dx_resource)
#define _CONTEXT _ABSTRACT(fsr_context)

#define FSR_ERROR_NOT_LOADED -1

namespace ffxFuncs {
static PfnFfxCreateContext CreateContext{};
static PfnFfxDestroyContext DestroyContext{};
static PfnFfxConfigure Configure{};
static PfnFfxQuery Query{};
static PfnFfxDispatch Dispatch{};
}

#define LOAD_FFX_FUNC(name) \
ffxFuncs::name = reinterpret_cast<PfnFfx##name>(GetProcAddress(mod, "ffx" #name))

#define CHECK_FFX_LOADED() \
if (ffxModule == nullptr) return FSR_ERROR_NOT_LOADED

static HMODULE ffxModule = nullptr;
static uint32_t ffxDebugLevel = FFX_API_CONFIGURE_GLOBALDEBUG_LEVEL_ERRORS;
static int liveContexts = 0;

static void onFfxMessage(uint32_t type, const wchar_t* message) {
    int len = WideCharToMultiByte(CP_UTF8, 0, message, -1, nullptr, 0, nullptr, nullptr);
    if (len <= 0)
        return;

    std::string utf8(len, '\0');
    WideCharToMultiByte(CP_UTF8, 0, message, -1, utf8.data(), len, nullptr, nullptr);
    printf("[FSR] %s: %s\n", type == FFX_API_MESSAGE_TYPE_ERROR ? "error" : "warning", utf8.c_str());
    fflush(stdout);
}

static void clearFfxFuncs() {
    ffxFuncs::CreateContext = nullptr;
    ffxFuncs::DestroyContext = nullptr;
    ffxFuncs::Configure = nullptr;
    ffxFuncs::Query = nullptr;
    ffxFuncs::Dispatch = nullptr;
}

static ffxCreateBackendDX12Desc makeBackendDesc(ID3D12Device* device) {
    ffxCreateBackendDX12Desc backendDesc{};
    backendDesc.header.type = FFX_API_CREATE_CONTEXT_DESC_TYPE_BACKEND_DX12;
    backendDesc.device = device;
    return backendDesc;
}

HL_PRIM int HL_NAME(init)(ID3D12Device* device, int debugLevel) {
    ffxDebugLevel = (uint32_t)debugLevel;
    if (ffxModule != nullptr)
        return FFX_API_RETURN_OK;

    wchar_t path[2048] = { 0 };
    DWORD len = GetModuleFileNameW(nullptr, path, _countof(path));
    if (len == 0)
        return FSR_ERROR_NOT_LOADED;

    std::filesystem::path dllPath = std::filesystem::path(path).parent_path() / L"amd_fidelityfx_loader_dx12.dll";
    HMODULE mod = LoadLibraryW(dllPath.c_str());
    if (mod == nullptr)
        return FSR_ERROR_NOT_LOADED;

    LOAD_FFX_FUNC(CreateContext);
    LOAD_FFX_FUNC(DestroyContext);
    LOAD_FFX_FUNC(Configure);
    LOAD_FFX_FUNC(Query);
    LOAD_FFX_FUNC(Dispatch);

    if (!ffxFuncs::CreateContext || !ffxFuncs::DestroyContext || !ffxFuncs::Configure || !ffxFuncs::Query || !ffxFuncs::Dispatch) {
        clearFfxFuncs();
        FreeLibrary(mod);
        return FSR_ERROR_NOT_LOADED;
    }

    uint64_t providerCount = 0;
    ffxQueryDescGetVersions versionsDesc{};
    versionsDesc.header.type = FFX_API_QUERY_DESC_TYPE_GET_VERSIONS;
    versionsDesc.createDescType = FFX_API_CREATE_CONTEXT_DESC_TYPE_UPSCALE;
    versionsDesc.device = device;
    versionsDesc.outputCount = &providerCount;

    ffxReturnCode_t res = ffxFuncs::Query(nullptr, &versionsDesc.header);
    if (res == FFX_API_RETURN_OK && providerCount == 0)
        res = FFX_API_RETURN_NO_PROVIDER;

    if (res != FFX_API_RETURN_OK) {
        clearFfxFuncs();
        FreeLibrary(mod);
        return (int)res;
    }

    ffxModule = mod;
    return FFX_API_RETURN_OK;
}

HL_PRIM int HL_NAME(shutdown)() {
    if (ffxModule == nullptr)
        return FFX_API_RETURN_OK;

    if (liveContexts > 0)
        return FFX_API_RETURN_ERROR;

    clearFfxFuncs();
    FreeLibrary(ffxModule);
    ffxModule = nullptr;
    return FFX_API_RETURN_OK;
}

HL_PRIM int HL_NAME(get_render_resolution)(ID3D12Device* device, int quality, int displayWidth, int displayHeight, int* outRenderWidth, int* outRenderHeight) {
    CHECK_FFX_LOADED();

    uint32_t renderWidth = 0;
    uint32_t renderHeight = 0;

    ffxCreateBackendDX12Desc backendDesc = makeBackendDesc(device);

    ffxQueryDescUpscaleGetRenderResolutionFromQualityMode desc{};
    desc.header.type = FFX_API_QUERY_DESC_TYPE_UPSCALE_GETRENDERRESOLUTIONFROMQUALITYMODE;
    desc.header.pNext = &backendDesc.header;
    desc.displayWidth = (uint32_t)displayWidth;
    desc.displayHeight = (uint32_t)displayHeight;
    desc.qualityMode = (uint32_t)quality;
    desc.pOutRenderWidth = &renderWidth;
    desc.pOutRenderHeight = &renderHeight;

    ffxReturnCode_t res = ffxFuncs::Query(nullptr, &desc.header);
    *outRenderWidth = (int)renderWidth;
    *outRenderHeight = (int)renderHeight;
    return (int)res;
}

HL_PRIM ffxContext HL_NAME(create_context)(ID3D12Device* device, int flags, int maxRenderWidth, int maxRenderHeight, int maxUpscaleWidth, int maxUpscaleHeight, int* outResult) {
    if (ffxModule == nullptr) {
        *outResult = FSR_ERROR_NOT_LOADED;
        return nullptr;
    }

    ffxCreateBackendDX12Desc backendDesc = makeBackendDesc(device);

    ffxCreateContextDescUpscaleVersion versionDesc{};
    versionDesc.header.type = FFX_API_CREATE_CONTEXT_DESC_TYPE_UPSCALE_VERSION;
    versionDesc.header.pNext = &backendDesc.header;
    versionDesc.version = FFX_UPSCALER_VERSION;

    ffxCreateContextDescUpscale desc{};
    desc.header.type = FFX_API_CREATE_CONTEXT_DESC_TYPE_UPSCALE;
    desc.header.pNext = &versionDesc.header;
    desc.flags = (uint32_t)flags;
    desc.maxRenderSize = { (uint32_t)maxRenderWidth, (uint32_t)maxRenderHeight };
    desc.maxUpscaleSize = { (uint32_t)maxUpscaleWidth, (uint32_t)maxUpscaleHeight };
    desc.fpMessage = onFfxMessage;

    ffxContext context = nullptr;
    ffxReturnCode_t res = ffxFuncs::CreateContext(&context, &desc.header, nullptr);
    *outResult = (int)res;
    if (res != FFX_API_RETURN_OK)
        return nullptr;

    ffxConfigureDescGlobalDebug1 debugDesc{};
    debugDesc.header.type = FFX_API_CONFIGURE_DESC_TYPE_GLOBALDEBUG1;
    debugDesc.fpMessage = onFfxMessage;
    debugDesc.debugLevel = ffxDebugLevel;
    ffxFuncs::Configure(&context, &debugDesc.header);

    liveContexts++;
    return context;
}

HL_PRIM int HL_NAME(destroy_context)(ffxContext context) {
    CHECK_FFX_LOADED();
    if (context == nullptr)
        return FFX_API_RETURN_ERROR_PARAMETER;

    ffxReturnCode_t res = ffxFuncs::DestroyContext(&context, nullptr);
    liveContexts--;
    return (int)res;
}

HL_PRIM vbyte* HL_NAME(get_version_name)(ffxContext context) {
    if (ffxModule == nullptr || context == nullptr)
        return nullptr;

    ffxQueryGetProviderVersion desc{};
    desc.header.type = FFX_API_QUERY_DESC_TYPE_GET_PROVIDER_VERSION;

    if (ffxFuncs::Query(&context, &desc.header) != FFX_API_RETURN_OK || desc.versionName == nullptr)
        return nullptr;

    return hl_copy_bytes((const vbyte*)desc.versionName, (int)strlen(desc.versionName) + 1);
}

struct FsrDispatchParams {
    ID3D12Resource* color;
    ID3D12Resource* depth;
    ID3D12Resource* motionVectors;
    ID3D12Resource* exposure;
    ID3D12Resource* reactive;
    ID3D12Resource* transparencyAndComposition;
    ID3D12Resource* output;
    D3D12_RESOURCE_STATES colorState;
    D3D12_RESOURCE_STATES depthState;
    D3D12_RESOURCE_STATES motionVectorsState;
    D3D12_RESOURCE_STATES exposureState;
    D3D12_RESOURCE_STATES reactiveState;
    D3D12_RESOURCE_STATES transparencyAndCompositionState;
    D3D12_RESOURCE_STATES outputState;
    float jitterOffsetX;
    float jitterOffsetY;
    float motionVectorScaleX;
    float motionVectorScaleY;
    int renderWidth;
    int renderHeight;
    int upscaleWidth;
    int upscaleHeight;
    float sharpness;
    float frameTimeDelta;
    float preExposure;
    float cameraNear;
    float cameraFar;
    float cameraFovAngleVertical;
    float viewSpaceToMetersFactor;
    int flags;
    bool enableSharpening;
    bool reset;
};

static uint32_t toFfxState(D3D12_RESOURCE_STATES state) {
    switch (state) {
    case D3D12_RESOURCE_STATE_COMMON: return FFX_API_RESOURCE_STATE_COMMON;
    case D3D12_RESOURCE_STATE_UNORDERED_ACCESS: return FFX_API_RESOURCE_STATE_UNORDERED_ACCESS;
    case D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE: return FFX_API_RESOURCE_STATE_COMPUTE_READ;
    case D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE: return FFX_API_RESOURCE_STATE_PIXEL_READ;
    case D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE: return FFX_API_RESOURCE_STATE_PIXEL_COMPUTE_READ;
    case D3D12_RESOURCE_STATE_COPY_SOURCE: return FFX_API_RESOURCE_STATE_COPY_SRC;
    case D3D12_RESOURCE_STATE_COPY_DEST: return FFX_API_RESOURCE_STATE_COPY_DEST;
    case D3D12_RESOURCE_STATE_GENERIC_READ: return FFX_API_RESOURCE_STATE_GENERIC_READ;
    case D3D12_RESOURCE_STATE_RENDER_TARGET: return FFX_API_RESOURCE_STATE_RENDER_TARGET;
    case D3D12_RESOURCE_STATE_DEPTH_WRITE: return FFX_API_RESOURCE_STATE_DEPTH_ATTACHMENT;
    default: return 0;
    }
}

static bool toFfxResource(ID3D12Resource* res, D3D12_RESOURCE_STATES state, FfxApiResource& out) {
    if (res == nullptr) {
        out = ffxApiGetResourceDX12(nullptr);
        return true;
    }

    uint32_t ffxState = toFfxState(state);
    if (ffxState == 0) {
        printf("[FSR] error: unsupported resource state 0x%x\n", (unsigned int)state);
        fflush(stdout);
        return false;
    }

    out = ffxApiGetResourceDX12(res, ffxState);
    return true;
}

HL_PRIM int HL_NAME(dispatch)(ffxContext context, ID3D12GraphicsCommandList* cmdList, FsrDispatchParams* params) {
    CHECK_FFX_LOADED();
    if (context == nullptr || cmdList == nullptr || params == nullptr)
        return FFX_API_RETURN_ERROR_PARAMETER;

    ffxDispatchDescUpscale desc{};
    desc.header.type = FFX_API_DISPATCH_DESC_TYPE_UPSCALE;
    desc.commandList = cmdList;

    if (!toFfxResource(params->color, params->colorState, desc.color)
        || !toFfxResource(params->depth, params->depthState, desc.depth)
        || !toFfxResource(params->motionVectors, params->motionVectorsState, desc.motionVectors)
        || !toFfxResource(params->exposure, params->exposureState, desc.exposure)
        || !toFfxResource(params->reactive, params->reactiveState, desc.reactive)
        || !toFfxResource(params->transparencyAndComposition, params->transparencyAndCompositionState, desc.transparencyAndComposition)
        || !toFfxResource(params->output, params->outputState, desc.output))
        return FFX_API_RETURN_ERROR_PARAMETER;

    desc.jitterOffset = { params->jitterOffsetX, params->jitterOffsetY };
    desc.motionVectorScale = { params->motionVectorScaleX, params->motionVectorScaleY };
    desc.renderSize = { (uint32_t)params->renderWidth, (uint32_t)params->renderHeight };
    desc.upscaleSize = { (uint32_t)params->upscaleWidth, (uint32_t)params->upscaleHeight };
    desc.enableSharpening = params->enableSharpening;
    desc.sharpness = params->sharpness;
    desc.frameTimeDelta = params->frameTimeDelta;
    desc.preExposure = params->preExposure;
    desc.reset = params->reset;
    desc.cameraNear = params->cameraNear;
    desc.cameraFar = params->cameraFar;
    desc.cameraFovAngleVertical = params->cameraFovAngleVertical;
    desc.viewSpaceToMetersFactor = params->viewSpaceToMetersFactor;
    desc.flags = (uint32_t)params->flags;

    return (int)ffxFuncs::Dispatch(&context, &desc.header);
}

DEFINE_PRIM(_I32, init, _DEVICE _I32);
DEFINE_PRIM(_I32, shutdown, _NO_ARG);
DEFINE_PRIM(_I32, get_render_resolution, _DEVICE _I32 _I32 _I32 _REF(_I32) _REF(_I32));
DEFINE_PRIM(_CONTEXT, create_context, _DEVICE _I32 _I32 _I32 _I32 _I32 _REF(_I32));
DEFINE_PRIM(_I32, destroy_context, _CONTEXT);
DEFINE_PRIM(_BYTES, get_version_name, _CONTEXT);
DEFINE_PRIM(_I32, dispatch, _CONTEXT _RES _STRUCT);
