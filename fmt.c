#define HL_NAME(n) fmt_##n
#include <png.h>
#include <hl.h>

#if defined(HL_CONSOLE) && !defined(HL_XBO)
extern bool sys_jpg_decode( vbyte *data, int dataLen, vbyte *out, int width, int height, int stride, int format, int flags );
#else
#	include <turbojpeg.h>
#endif

#include <vorbis/vorbisfile.h>

#define MINIMP3_IMPLEMENTATION
#define MINIMP3_FLOAT_OUTPUT
#include <minimp3.h>

/* ------------------------------------------------- IMG --------------------------------------------------- */

typedef struct {
	unsigned char a,r,g,b;
} pixel;

HL_PRIM bool HL_NAME(jpg_decode)( vbyte *data, int dataLen, vbyte *out, int width, int height, int stride, int format, int flags ) {
#if defined(HL_CONSOLE) && !defined(HL_XBO)
	hl_blocking(true);
	bool b = sys_jpg_decode(data, dataLen, out, width, height, stride, format, flags);
	hl_blocking(false);
	return b;
#else
	hl_blocking(true);
	tjhandle h = tjInitDecompress();
	int result;
	result = tjDecompress2(h,data,dataLen,out,width,stride,height,format,(flags & 1 ? TJFLAG_BOTTOMUP : 0));
	tjDestroy(h);
	hl_blocking(false);
	return result == 0;
#endif
}

HL_PRIM bool HL_NAME(png_decode)( vbyte *data, int dataLen, vbyte *out, int width, int height, int stride, int format, int flags ) {
#	ifdef PNG_IMAGE_VERSION
	png_image img;
	hl_blocking(true);
	memset(&img, 0, sizeof(img));
	img.version = PNG_IMAGE_VERSION;
	if( png_image_begin_read_from_memory(&img,data,dataLen) == 0 ) {
		hl_blocking(false);
		png_image_free(&img);
		return false;
	}
	switch( format ) {
	case 0:
		img.format = PNG_FORMAT_RGB;
		break;
	case 1:
		img.format = PNG_FORMAT_BGR;
		break;
	case 7:
		img.format = PNG_FORMAT_RGBA;
		break;
	case 8:
		img.format = PNG_FORMAT_BGRA;
		break;
	case 9:
		img.format = PNG_FORMAT_ABGR;
		break;
	case 10:
		img.format = PNG_FORMAT_ARGB;
		break;
	case 12:
		img.format = PNG_FORMAT_LINEAR_Y;
		break;
	case 13:
		img.format = PNG_FORMAT_LINEAR_RGB;
		break;
	case 14:
		img.format = PNG_FORMAT_LINEAR_RGB_ALPHA;
		break;
	case 15:
		img.format = PNG_FORMAT_LINEAR_Y_ALPHA;
		break;
	default:
		hl_blocking(false);
		png_image_free(&img);
		hl_error("Unsupported format");
		break;
	}
	if( img.width != width || img.height != height ) {
		hl_blocking(false);
		png_image_free(&img);
		return false;
	}
	if( png_image_finish_read(&img,NULL,out,stride * (flags & 1 ? -1 : 1),NULL) == 0 ) {
		hl_blocking(false);
		png_image_free(&img);
		return false;
	}
	hl_blocking(false);
	png_image_free(&img);
#	else
	hl_error("PNG support is missing for this libPNG version");
#	endif
	return true;
}

HL_PRIM void HL_NAME(img_scale)( vbyte *out, int outPos, int outStride, int outWidth, int outHeight, vbyte *in, int inPos, int inStride, int inWidth, int inHeight, int flags ) {
	int x, y;
	float scaleX = outWidth <= 1 ? 0.0f : (float)((inWidth - 1.001f) / (outWidth - 1));
	float scaleY = outHeight <= 1 ? 0.0f : (float)((inHeight - 1.001f) / (outHeight - 1));
	out += outPos;
	in += inPos;
	hl_blocking(true);
	for(y=0;y<outHeight;y++) {
		for(x=0;x<outWidth;x++) {
			float fx = x * scaleX;
			float fy = y * scaleY;
			int ix = (int)fx;
			int iy = (int)fy;
			if( (flags & 1) == 0 ) {
				// nearest
				vbyte *rin = in + iy * inStride;
				*(pixel*)out = *(pixel*)(rin + (ix<<2));
				out += 4;
			} else {
				// bilinear
				float rx = fx - ix;
				float ry = fy - iy;
				float rx1 = 1.0f - rx;
				float ry1 = 1.0f - ry;
				int w1 = (int)(rx1 * ry1 * 256.0f);
				int w2 = (int)(rx * ry1 * 256.0f);
				int w3 = (int)(rx1 * ry * 256.0f);
				int w4 = (int)(rx * ry * 256.0f);
				vbyte *rin = in + iy * inStride;
				pixel p1 = *(pixel*)(rin + (ix<<2));
				pixel p2 = *(pixel*)(rin + ((ix + 1)<<2));
				pixel p3 = *(pixel*)(rin + inStride + (ix<<2));
				pixel p4 = *(pixel*)(rin + inStride + ((ix + 1)<<2));
				*out++ = (unsigned char)((p1.a * w1 + p2.a * w2 + p3.a * w3 + p4.a * w4 + 128)>>8);
				*out++ = (unsigned char)((p1.r * w1 + p2.r * w2 + p3.r * w3 + p4.r * w4 + 128)>>8);
				*out++ = (unsigned char)((p1.g * w1 + p2.g * w2 + p3.g * w3 + p4.g * w4 + 128)>>8);
				*out++ = (unsigned char)((p1.b * w1 + p2.b * w2 + p3.b * w3 + p4.b * w4 + 128)>>8);
			}
		}
		out += outStride - (outWidth << 2);
	}
	hl_blocking(false);
}


DEFINE_PRIM(_BOOL, jpg_decode, _BYTES _I32 _BYTES _I32 _I32 _I32 _I32 _I32);
DEFINE_PRIM(_BOOL, png_decode, _BYTES _I32 _BYTES _I32 _I32 _I32 _I32 _I32);
DEFINE_PRIM(_VOID, img_scale, _BYTES _I32 _I32 _I32 _I32 _BYTES _I32 _I32 _I32 _I32 _I32);


/* ----------------------------------------------- SOUND : OGG ------------------------------------------------ */

typedef struct _fmt_ogg fmt_ogg;
struct _fmt_ogg {
	void (*finalize)( fmt_ogg * );
	OggVorbis_File f;
	char *bytes;
	int pos;
	int size;
	int section;
};

static void ogg_finalize( fmt_ogg *o ) {
	ov_clear(&o->f);
}

static size_t ogg_memread( void *ptr, int size, int count, fmt_ogg *o ) {
	int len = size * count;
	if( o->pos + len > o->size )
		len = o->size - o->pos;
	memcpy(ptr, o->bytes + o->pos, len);
	o->pos += len;
	return len;
}

static int ogg_memseek( fmt_ogg *o, ogg_int64_t _offset, int mode ) {
	int offset = (int)_offset;
	switch( mode ) {
	case SEEK_SET:
		if( offset < 0 || offset > o->size ) return 1;
		o->pos = offset;
		break;
	case SEEK_CUR:
		if( o->pos + offset < 0 || o->pos + offset > o->size ) return 1;
		o->pos += offset;
		break;
	case SEEK_END:
		if( offset < 0 || offset > o->size ) return 1;
		o->pos = o->size - offset;
		break;
	}
	return 0;
}

static long ogg_memtell( fmt_ogg *o ) {
	return o->pos;
}

static ov_callbacks OV_CALLBACKS_MEMORY = {
  (size_t (*)(void *, size_t, size_t, void *))  ogg_memread,
  (int (*)(void *, ogg_int64_t, int))           ogg_memseek,
  (int (*)(void *))                             NULL,
  (long (*)(void *))                            ogg_memtell
};

HL_PRIM fmt_ogg *HL_NAME(ogg_open)( char *bytes, int size ) {
	fmt_ogg *o = (fmt_ogg*)hl_gc_alloc_finalizer(sizeof(fmt_ogg));
	o->finalize = NULL;
	o->bytes = bytes;
	o->size = size;
	o->pos = 0;
	if( ov_open_callbacks(o,&o->f,NULL,0,OV_CALLBACKS_MEMORY) != 0 )
		return NULL;
	o->finalize = ogg_finalize;
	return o;
}

HL_PRIM void HL_NAME(ogg_info)( fmt_ogg *o, int *bitrate, int *freq, int *samples, int *channels ) {
	vorbis_info *i = ov_info(&o->f,-1);
	*bitrate = i->bitrate_nominal;
	*freq = i->rate;
	*channels = i->channels;
	*samples = (int)ov_pcm_total(&o->f, -1);
}

HL_PRIM int HL_NAME(ogg_tell)( fmt_ogg *o ) {
	return (int)ov_pcm_tell(&o->f); // overflow at 12 hours @48 Khz
}

HL_PRIM bool HL_NAME(ogg_seek)( fmt_ogg *o, int sample ) {
	return ov_pcm_seek(&o->f,sample) == 0;
}

#define OGGFMT_I8			1
#define OGGFMT_I16			2
//#define OGGFMT_F32		3
#define OGGFMT_BIGENDIAN	128
#define OGGFMT_UNSIGNED		256

HL_PRIM int HL_NAME(ogg_read)( fmt_ogg *o, char *output, int size, int format ) {
	int ret = -1;
	hl_blocking(true);
	switch( format&127 ) {
	case OGGFMT_I8:
	case OGGFMT_I16:
		ret = ov_read(&o->f, output, size, (format & OGGFMT_BIGENDIAN) != 0, format&3, (format & OGGFMT_UNSIGNED) == 0, &o->section);
		break;
//	case OGGFMT_F32:
//		-- this decodes separates channels instead of mixed single buffer one
//		return ov_read_float(&o->f, output, size, (format & OGGFMT_BIGENDIAN) != 0, format&3, (format & OGGFMT_UNSIGNED) == 0, &o->section);
	default:
		break;
	}
	hl_blocking(false);
	return ret;
}

#define _OGG _ABSTRACT(fmt_ogg)

DEFINE_PRIM(_OGG, ogg_open, _BYTES _I32);
DEFINE_PRIM(_VOID, ogg_info, _OGG _REF(_I32) _REF(_I32) _REF(_I32) _REF(_I32));
DEFINE_PRIM(_I32, ogg_tell, _OGG);
DEFINE_PRIM(_BOOL, ogg_seek, _OGG _I32);
DEFINE_PRIM(_I32, ogg_read, _OGG _BYTES _I32 _I32);

/* ----------------------------------------------- SOUND : MP3 ------------------------------------------------ */

typedef struct _fmt_mp3 fmt_mp3;
struct _fmt_mp3 {
	mp3dec_t dec;
	mp3dec_frame_info_t info;
	mp3d_sample_t pcm[MINIMP3_MAX_SAMPLES_PER_FRAME];
};

// Allocate MP3 reader.
HL_PRIM fmt_mp3 *HL_NAME(mp3_open)() {
	fmt_mp3 *o = (fmt_mp3*)hl_gc_alloc_noptr(sizeof(fmt_mp3));
	mp3dec_init(&o->dec);
	return o;
}

/**
	Retreive last decoded frame information.
	@param bitrate_kbps Bitrate of the frame
	@param channels Total amount of channels in the frame.
	@param frame_bytes The size of the frame in the input stream,
	@param hz
	@param layer Mpeg Layer index (usually 3).
**/
HL_PRIM void HL_NAME(mp3_frame_info)(fmt_mp3 *o, int *bitrate_kbps, int *channels, int *frame_bytes, int *hz, int *layer) {
	*bitrate_kbps = o->info.bitrate_kbps;
	*channels = o->info.channels;
	*frame_bytes = o->info.frame_bytes;
	*hz = o->info.hz;
	*layer = o->info.layer;
}

/**
	Decodes a single frame from input stream and writes result to output.
	Decoded samples are in Float32 format. Output bytes should contain enough space to fit entire frame in.
	To calculate required output size, follow next formula: `samples * channels * 4`.
	For Layer 1, amount of frames is 384, MPEG 2 Layer 2 is 576 and 1152 otherwise. Using 1152 samples is the safest.
	@param o Allocated MP3 reader.
	@param bytes Input stream.
	@param size Input stream size.
	@param position Input stream offset.
	@param output Output stream.
	@param outputSize Output stream size.
	@param offset Output stream write offset.
	@returns 0 if no MP3 data was found (end of stream/invalid data), -1 if either input buffer position invalid or output size is insufficent.
		Amount of decoded samples otherwise.
**/
HL_PRIM int HL_NAME(mp3_decode_frame)( fmt_mp3 *o, char *bytes, int size, int position, char *output, int outputSize, int offset ) {

	// Out of mp3 file bounds.
	if ( position < 0 || size <= position )
		return -1;

	int samples = 0;
	hl_blocking(true);

	do {
		samples = mp3dec_decode_frame(&o->dec, (unsigned char*)bytes + position, size - position, o->pcm, &o->info);
		// Try to read until found mp3 data or EOF.
		if ( samples != 0 || o->info.frame_bytes == 0 )
			break;
		position += o->info.frame_bytes;
	} while ( size > position );

	// No or invalid MP3 data.
	if ( samples == 0 || o->info.frame_bytes == 0 ) {
		hl_blocking(false);
		return 0;
	}

	int decodedSize = samples * o->info.channels * sizeof(mp3d_sample_t);
	// Insufficent output buffer size.
	if ( outputSize - offset < decodedSize ) {
		hl_blocking(false);
		return -1;
	}

	memcpy( (void *)(output + offset), (void *)o->pcm, decodedSize );

	hl_blocking(false);
	return samples;
}

#define _MP3 _ABSTRACT(fmt_mp3)

DEFINE_PRIM(_MP3, mp3_open, _NO_ARG);
DEFINE_PRIM(_VOID, mp3_frame_info, _MP3 _REF(_I32) _REF(_I32) _REF(_I32) _REF(_I32) _REF(_I32))
DEFINE_PRIM(_I32, mp3_decode_frame, _MP3 _BYTES _I32 _I32 _BYTES _I32 _I32);