package heaps.fsr;

#if (hldx && dx12 && fsr)

typedef FsrDevice = dx.Dx12.Device;
typedef FsrRes = dx.Dx12.Resource;
typedef FsrResourceState = dx.Dx12.ResourceState;
typedef FsrCommandList = dx.Dx12.CommandList;
typedef FsrCommandQueue = dx.Dx12.CommandQueue;
typedef FsrFactory = dx.Dx12.Factory;
typedef FsrDxgiFormat = dx.Dx12.DxgiFormat;

typedef FsrContext = hl.Abstract<"fsr_context">;
typedef FsrSwapChain = hl.Abstract<"dx_swapchain">;
typedef FsrWindow = hl.Abstract<"dx_window">;

enum abstract FsrResult(Int) from Int to Int {
	var InvalidState = -3;
	var NotCreated = -2;
	var NotLoaded = -1;
	var Ok = 0;
	var Error = 1;
	var ErrorUnknownDescType = 2;
	var ErrorRuntimeError = 3;
	var NoProvider = 4;
	var ErrorMemory = 5;
	var ErrorParameter = 6;
	var ProviderNoSupportNewDescType = 7;
}

enum abstract FsrDebugLevel(Int) to Int {
	public var SILENCE = 0;
	public var ERRORS = 1;
	public var WARNINGS = 2;
	public var VERBOSE = 0xfffffff;
}

enum abstract FsrQuality(Int) to Int {
	public var NATIVE_AA = 0;
	public var QUALITY = 1;
	public var BALANCED = 2;
	public var PERFORMANCE = 3;
	public var ULTRA_PERFORMANCE = 4;
}

enum abstract FsrCreateFlag(Int) to Int {
	public var HIGH_DYNAMIC_RANGE = 1;
	public var DISPLAY_RESOLUTION_MOTION_VECTORS = 2;
	public var MOTION_VECTORS_JITTER_CANCELLATION = 4;
	public var DEPTH_INVERTED = 8;
	public var DEPTH_INFINITE = 16;
	public var AUTO_EXPOSURE = 32;
	public var DYNAMIC_RESOLUTION = 64;
	public var DEBUG_CHECKING = 128;
	public var NON_LINEAR_COLORSPACE = 256;
	public var DEBUG_VISUALIZATION = 512;
}

enum abstract FsrDispatchFlag(Int) to Int {
	public var DRAW_DEBUG_VIEW = 1;
	public var NON_LINEAR_COLOR_SRGB = 2;
	public var NON_LINEAR_COLOR_PQ = 4;
}

enum abstract FsrEffect(Int) to Int {
	public var UPSCALE = 0;
	public var FRAME_GENERATION = 1;
}

enum abstract FsrFrameGenCreateFlag(Int) to Int {
	public var ASYNC_WORKLOAD_SUPPORT = 1;
	public var DISPLAY_RESOLUTION_MOTION_VECTORS = 2;
	public var MOTION_VECTORS_JITTER_CANCELLATION = 4;
	public var DEPTH_INVERTED = 8;
	public var DEPTH_INFINITE = 16;
	public var HIGH_DYNAMIC_RANGE = 32;
	public var DEBUG_CHECKING = 64;
}

enum abstract FsrFrameGenFlag(Int) to Int {
	public var DRAW_DEBUG_TEAR_LINES = 1;
	public var DRAW_DEBUG_RESET_INDICATORS = 2;
	public var DRAW_DEBUG_VIEW = 4;
	public var NO_SWAPCHAIN_CONTEXT_NOTIFY = 8;
	public var DRAW_DEBUG_PACING_LINES = 16;
}

enum abstract FsrUiCompositionFlag(Int) to Int {
	public var USE_PREMUL_ALPHA = 1;
	public var ENABLE_INTERNAL_UI_DOUBLE_BUFFERING = 2;
}

@:struct class FsrDispatchParams {
	public var color : FsrRes;
	public var depth : FsrRes;
	public var motionVectors : FsrRes;
	public var exposure : FsrRes;
	public var reactive : FsrRes;
	public var transparencyAndComposition : FsrRes;
	public var output : FsrRes;
	public var colorState : FsrResourceState;
	public var depthState : FsrResourceState;
	public var motionVectorsState : FsrResourceState;
	public var exposureState : FsrResourceState;
	public var reactiveState : FsrResourceState;
	public var transparencyAndCompositionState : FsrResourceState;
	public var outputState : FsrResourceState;
	public var jitterOffsetX : Single;
	public var jitterOffsetY : Single;
	public var motionVectorScaleX : Single;
	public var motionVectorScaleY : Single;
	public var renderWidth : Int;
	public var renderHeight : Int;
	public var upscaleWidth : Int;
	public var upscaleHeight : Int;
	public var sharpness : Single;
	public var frameTimeDelta : Single;
	public var preExposure : Single;
	public var cameraNear : Single;
	public var cameraFar : Single;
	public var cameraFovAngleVertical : Single;
	public var viewSpaceToMetersFactor : Single;
	public var flags : Int;
	public var enableSharpening : Bool;
	public var reset : Bool;
	public function new() {
	}
}

@:struct class FsrFrameGenConfig {
	public var swapChain : FsrSwapChain;
	public var hudless : FsrRes;
	public var hudlessState : FsrResourceState;
	public var flags : Int;
	public var rectX : Int;
	public var rectY : Int;
	public var rectWidth : Int;
	public var rectHeight : Int;
	public var frameID : Int;
	public var enabled : Bool;
	public var allowAsyncWorkloads : Bool;
	public var onlyPresentGenerated : Bool;
	public function new() {
	}
}

@:struct class FsrFrameGenPrepareParams {
	public var depth : FsrRes;
	public var motionVectors : FsrRes;
	public var depthState : FsrResourceState;
	public var motionVectorsState : FsrResourceState;
	public var renderWidth : Int;
	public var renderHeight : Int;
	public var jitterOffsetX : Single;
	public var jitterOffsetY : Single;
	public var motionVectorScaleX : Single;
	public var motionVectorScaleY : Single;
	public var frameTimeDelta : Single;
	public var cameraNear : Single;
	public var cameraFar : Single;
	public var cameraFovAngleVertical : Single;
	public var viewSpaceToMetersFactor : Single;
	public var cameraPositionX : Single;
	public var cameraPositionY : Single;
	public var cameraPositionZ : Single;
	public var cameraUpX : Single;
	public var cameraUpY : Single;
	public var cameraUpZ : Single;
	public var cameraRightX : Single;
	public var cameraRightY : Single;
	public var cameraRightZ : Single;
	public var cameraForwardX : Single;
	public var cameraForwardY : Single;
	public var cameraForwardZ : Single;
	public var frameID : Int;
	public var flags : Int;
	public var reset : Bool;
	public function new() {
	}
}

@:hlNative("fsr")
class Fsr {

	public static function init(device : FsrDevice, debugLevel : FsrDebugLevel) : Int {
		return 0;
	}

	public static function shutdown() : Int {
		return 0;
	}

	public static function getRenderResolution(device : FsrDevice, quality : FsrQuality, displayWidth : Int, displayHeight : Int, renderWidth : hl.Ref<Int>, renderHeight : hl.Ref<Int>) : Int {
		return 0;
	}

	public static function createContext(device : FsrDevice, flags : Int, maxRenderWidth : Int, maxRenderHeight : Int, maxUpscaleWidth : Int, maxUpscaleHeight : Int, result : hl.Ref<Int>) : FsrContext {
		return null;
	}

	public static function destroyContext(context : FsrContext) : Int {
		return 0;
	}

	public static function dispatch(context : FsrContext, commandList : FsrCommandList, params : FsrDispatchParams) : Int {
		return 0;
	}

	static function getVersionName(context : FsrContext) : hl.Bytes {
		return null;
	}

	public static function getVersion(context : FsrContext) : String {
		var bytes = getVersionName(context);
		return bytes == null ? null : @:privateAccess String.fromUTF8(bytes);
	}

	public static function getEffectVersionCount(device : FsrDevice, effect : FsrEffect, count : hl.Ref<Int>) : Int {
		return 0;
	}

	static function getEffectVersionName(device : FsrDevice, effect : FsrEffect, index : Int) : hl.Bytes {
		return null;
	}

	public static function getEffectVersion(device : FsrDevice, effect : FsrEffect, index : Int = 0) : String {
		var bytes = getEffectVersionName(device, effect, index);
		return bytes == null ? null : @:privateAccess String.fromUTF8(bytes);
	}

	public static function isEffectAvailable(device : FsrDevice, effect : FsrEffect) : Bool {
		var count = 0;
		return getEffectVersionCount(device, effect, count) == FsrResult.Ok && count > 0;
	}

	public static function createSwapChain(window : FsrWindow, nativeFactory : FsrFactory, nativeQueue : FsrCommandQueue, width : Int, height : Int, bufferCount : Int, format : FsrDxgiFormat, result : hl.Ref<Int>) : FsrSwapChain {
		return null;
	}

	public static function destroySwapChainContext() : Int {
		return 0;
	}

	public static function releaseSwapChain(swapChain : FsrSwapChain) : Int {
		return 0;
	}

	public static function waitForPresents() : Int {
		return 0;
	}

	public static function registerUiResource(resource : FsrRes, state : FsrResourceState, flags : Int) : Int {
		return 0;
	}

	public static function setFramePacingTuning(safetyMarginInMs : Single, varianceFactor : Single, allowHybridSpin : Bool, hybridSpinTime : Int, allowWaitForSingleObjectOnFence : Bool) : Int {
		return 0;
	}

	public static function querySwapChainMemory(totalBytes : hl.Ref<Float>, aliasableBytes : hl.Ref<Float>) : Int {
		return 0;
	}

	public static function createFrameGenContext(device : FsrDevice, flags : Int, displayWidth : Int, displayHeight : Int, maxRenderWidth : Int, maxRenderHeight : Int, backBufferFormat : FsrDxgiFormat, hudlessFormat : FsrDxgiFormat, result : hl.Ref<Int>) : FsrContext {
		return null;
	}

	public static function configureFrameGen(context : FsrContext, config : FsrFrameGenConfig) : Int {
		return 0;
	}

	public static function dispatchFrameGenPrepare(context : FsrContext, commandList : FsrCommandList, params : FsrFrameGenPrepareParams) : Int {
		return 0;
	}

	public static function queryFrameGenMemory(context : FsrContext, totalBytes : hl.Ref<Float>, aliasableBytes : hl.Ref<Float>) : Int {
		return 0;
	}

	@:hlNative("fsr", "antilag2_init")
	public static function antiLag2Init(device : FsrDevice) : Int {
		return 0;
	}

	@:hlNative("fsr", "antilag2_deinit")
	public static function antiLag2DeInit() : Int {
		return 0;
	}

	@:hlNative("fsr", "antilag2_update")
	public static function antiLag2Update(enable : Bool, maxFps : Int) : Int {
		return 0;
	}

	@:hlNative("fsr", "antilag2_present")
	public static function antiLag2Present(swapChain : FsrSwapChain, enabled : Bool) : Int {
		return 0;
	}
}

#end
