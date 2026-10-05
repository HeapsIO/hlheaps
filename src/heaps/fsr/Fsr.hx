package heaps.fsr;

#if (hldx && dx12 && fsr)

typedef FsrDevice = dx.Dx12.Device;
typedef FsrRes = dx.Dx12.Resource;
typedef FsrResourceState = dx.Dx12.ResourceState;
typedef FsrCommandList = dx.Dx12.CommandList;

typedef FsrContext = hl.Abstract<"fsr_context">;

enum abstract FsrResult(Int) from Int to Int {
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
}

#end
