// as_netstream.cpp	-- Vitaly Alexeev <vitaly.alexeev@yahoo.com> 200
// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "bakeinflash/bakeinflash_root.h"
#include "bakeinflash/bakeinflash_as_classes/as_camera.h"
#include "bakeinflash/bakeinflash_render.h"
#include "bakeinflash/bakeinflash_impl.h"
#include "base/tu_file.h"
#include "base/tu_random.h"
#include "jpeglib/jpeglib.h"
#include <setjmp.h>


#if TU_CONFIG_LINK_TO_V4L2 == 1

#include <getopt.h>             /* getopt_long() */
#include <fcntl.h>              /* low-level i/o */
#include <unistd.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <linux/videodev2.h>

#endif

#ifdef iOS
	#import "as_camera_.h"

	@interface EAGLView : UIView <UITextFieldDelegate> @end
	extern EAGLView* s_view;
#endif

#if TU_CONFIG_LINK_TO_V4L2 == 1

	static struct v4l2_format fmt = {};

// This callback function runs once per frame. Use it to perform any
// quick processing you need, or have it put the frame into your application's
// input queue. If this function takes too long, you'll start losing frames.
static inline unsigned char sat(int i) {
  return (unsigned char)( i >= 255 ? 255 : (i < 0 ? 0 : i));
}

#define IYUYV2RGB_2(pyuv, prgb) { \
    int r = (22987 * ((pyuv)[3] - 128)) >> 14; \
    int g = (-5636 * ((pyuv)[1] - 128) - 11698 * ((pyuv)[3] - 128)) >> 14; \
    int b = (29049 * ((pyuv)[1] - 128)) >> 14; \
    (prgb)[0] = sat(*(pyuv) + r); \
    (prgb)[1] = sat(*(pyuv) + g); \
    (prgb)[2] = sat(*(pyuv) + b); \
    (prgb)[3] = sat((pyuv)[2] + r); \
    (prgb)[4] = sat((pyuv)[2] + g); \
    (prgb)[5] = sat((pyuv)[2] + b); \
    }
#define IYUYV2RGB_16(pyuv, prgb) IYUYV2RGB_8(pyuv, prgb); IYUYV2RGB_8(pyuv + 16, prgb + 24);
#define IYUYV2RGB_8(pyuv, prgb) IYUYV2RGB_4(pyuv, prgb); IYUYV2RGB_4(pyuv + 8, prgb + 12);
#define IYUYV2RGB_4(pyuv, prgb) IYUYV2RGB_2(pyuv, prgb); IYUYV2RGB_2(pyuv + 4, prgb + 6);

const Uint8* my_uvc_yuyv2rgb(Uint8* data, int size)
{

  uint8_t *pyuv = NULL;
  uint8_t *prgb = NULL;
  uint8_t *prgb_end = NULL;

  int data_bytes = fmt.fmt.pix.width * fmt.fmt.pix.height * 3;
  Uint8* out = (Uint8*) malloc(data_bytes);
  pyuv = data;
  prgb = (uint8_t*) out;
  prgb_end = prgb + data_bytes;

  while (prgb < prgb_end)
  {
    IYUYV2RGB_8(pyuv, prgb);
    prgb += 3 * 8;
    pyuv += 2 * 8;
  }

	return out;
}


#define CLEAR(x) memset(&(x), 0, sizeof(x))

enum io_method {
	IO_METHOD_READ,
	IO_METHOD_MMAP,
	IO_METHOD_USERPTR,
};

struct buffer {
	void   *start;
	size_t  length;
};

static char            *dev_name = "/dev/video0";
static enum io_method   io = IO_METHOD_MMAP;
static int              fd = -1;
struct buffer          *buffers;
static unsigned int     n_buffers;
static int              out_buf;
static int              force_format=0;
static int              frame_number = 0;

static void errno_exit(const char *s)
{
	printf("%s error %d, %s\n", s, errno, strerror(errno));
	exit(EXIT_FAILURE);
}

static int xioctl(int fh, int request, void *arg)
{
	int r;

	do {
		r = ioctl(fh, request, arg);
	} while (-1 == r && EINTR == errno);

	return r;
}

/*static void process_image(const void *p, int size)
{
	frame_number++;
	char filename[15];
	sprintf(filename, "frame-%d.raw", frame_number);
	FILE *fp=fopen(filename,"wb");

	if (out_buf)
		fwrite(p, size, 1, fp);

	fflush(fp);
	fclose(fp);
}*/

static int read_frame(Uint8** data, int* size)
{
	struct v4l2_buffer buf;
	unsigned int i;

	switch (io)
	{
	case IO_METHOD_READ:
		if (-1 == read(fd, buffers[0].start, buffers[0].length)) 
		{
			switch (errno) {
			case EAGAIN:
				return 0;

			case EIO:
				/* Could ignore EIO, see spec. */

				/* fall through */

			default:
				errno_exit("read");
			}
		}

		//process_image(buffers[0].start, buffers[0].length);
		*data = (Uint8*) buffers[0].start;
		*size = buffers[0].length;
		break;

	case IO_METHOD_MMAP:
		CLEAR(buf);

		buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		buf.memory = V4L2_MEMORY_MMAP;

		if (-1 == xioctl(fd, VIDIOC_DQBUF, &buf)) {
			switch (errno) {
			case EAGAIN:
				return 0;

			case EIO:
				/* Could ignore EIO, see spec. */

				/* fall through */

			default:
				errno_exit("VIDIOC_DQBUF");
			}
		}

		assert(buf.index < n_buffers);

		//process_image(buffers[buf.index].start, buf.bytesused);
		*data = (Uint8*) buffers[buf.index].start;
		*size = buf.bytesused;

		if (-1 == xioctl(fd, VIDIOC_QBUF, &buf))
		{
			errno_exit("VIDIOC_QBUF");
		}
		break;

	case IO_METHOD_USERPTR:
		CLEAR(buf);

		buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		buf.memory = V4L2_MEMORY_USERPTR;

		if (-1 == xioctl(fd, VIDIOC_DQBUF, &buf)) {
			switch (errno) {
			case EAGAIN:
				return 0;

			case EIO:
				/* Could ignore EIO, see spec. */

				/* fall through */

			default:
				errno_exit("VIDIOC_DQBUF");
			}
		}

		for (i = 0; i < n_buffers; ++i)
			if (buf.m.userptr == (unsigned long)buffers[i].start
				&& buf.length == buffers[i].length)
				break;

		assert(i < n_buffers);

		//process_image((void *)buf.m.userptr, buf.bytesused);
		*data = (Uint8*) buf.m.userptr;
		*size = buf.bytesused;

		if (-1 == xioctl(fd, VIDIOC_QBUF, &buf))
		{
			errno_exit("VIDIOC_QBUF");
		}
		break;
	}

	return 1;
}

static void stop_capturing(void)
{
	enum v4l2_buf_type type;

	switch (io) {
	case IO_METHOD_READ:
		/* Nothing to do. */
		break;

	case IO_METHOD_MMAP:
	case IO_METHOD_USERPTR:
		type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		if (-1 == xioctl(fd, VIDIOC_STREAMOFF, &type))
			errno_exit("VIDIOC_STREAMOFF");
		break;
	}
}

static void start_capturing(void)
{
	unsigned int i;
	enum v4l2_buf_type type;

	switch (io) {
	case IO_METHOD_READ:
		/* Nothing to do. */
		break;

	case IO_METHOD_MMAP:
		for (i = 0; i < n_buffers; ++i) {
			struct v4l2_buffer buf;

			CLEAR(buf);
			buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
			buf.memory = V4L2_MEMORY_MMAP;
			buf.index = i;

			if (-1 == xioctl(fd, VIDIOC_QBUF, &buf))
				errno_exit("VIDIOC_QBUF");
		}
		type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		if (-1 == xioctl(fd, VIDIOC_STREAMON, &type))
			errno_exit("VIDIOC_STREAMON");
		break;

	case IO_METHOD_USERPTR:
		for (i = 0; i < n_buffers; ++i) {
			struct v4l2_buffer buf;

			CLEAR(buf);
			buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
			buf.memory = V4L2_MEMORY_USERPTR;
			buf.index = i;
			buf.m.userptr = (unsigned long)buffers[i].start;
			buf.length = buffers[i].length;

			if (-1 == xioctl(fd, VIDIOC_QBUF, &buf))
				errno_exit("VIDIOC_QBUF");
		}
		type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		if (-1 == xioctl(fd, VIDIOC_STREAMON, &type))
			errno_exit("VIDIOC_STREAMON");
		break;
	}
}

static void uninit_device(void)
{
	unsigned int i;

	switch (io) {
	case IO_METHOD_READ:
		free(buffers[0].start);
		break;

	case IO_METHOD_MMAP:
		for (i = 0; i < n_buffers; ++i)
			if (-1 == munmap(buffers[i].start, buffers[i].length))
				errno_exit("munmap");
		break;

	case IO_METHOD_USERPTR:
		for (i = 0; i < n_buffers; ++i)
			free(buffers[i].start);
		break;
	}

	free(buffers);
}

static void init_read(unsigned int buffer_size)
{
	buffers = (buffer*) calloc(1, sizeof(*buffers));

	if (!buffers) {
		printf("Out of memory\n");
		exit(EXIT_FAILURE);
	}

	buffers[0].length = buffer_size;
	buffers[0].start = malloc(buffer_size);

	if (!buffers[0].start) {
		printf("Out of memory\n");
		exit(EXIT_FAILURE);
	}
}

static void init_mmap(void)
{
	struct v4l2_requestbuffers req;

	CLEAR(req);

	req.count = 4;
	req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	req.memory = V4L2_MEMORY_MMAP;

	if (-1 == xioctl(fd, VIDIOC_REQBUFS, &req)) {
		if (EINVAL == errno) {
			printf("%s does not support "
				"memory mapping\n", dev_name);
			exit(EXIT_FAILURE);
		} else {
			errno_exit("VIDIOC_REQBUFS");
		}
	}

	if (req.count < 2) {
		printf("Insufficient buffer memory on %s\n",
			dev_name);
		exit(EXIT_FAILURE);
	}

	buffers = (buffer*) calloc(req.count, sizeof(*buffers));

	if (!buffers) {
		printf("Out of memory\n");
		exit(EXIT_FAILURE);
	}

	for (n_buffers = 0; n_buffers < req.count; ++n_buffers) {
		struct v4l2_buffer buf;

		CLEAR(buf);

		buf.type        = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		buf.memory      = V4L2_MEMORY_MMAP;
		buf.index       = n_buffers;

		if (-1 == xioctl(fd, VIDIOC_QUERYBUF, &buf))
			errno_exit("VIDIOC_QUERYBUF");

		buffers[n_buffers].length = buf.length;
		buffers[n_buffers].start =
			mmap(NULL /* start anywhere */,
			buf.length,
			PROT_READ | PROT_WRITE /* required */,
			MAP_SHARED /* recommended */,
			fd, buf.m.offset);

		if (MAP_FAILED == buffers[n_buffers].start)
			errno_exit("mmap");
	}
}

static void init_userp(unsigned int buffer_size)
{
	struct v4l2_requestbuffers req;

	CLEAR(req);

	req.count  = 4;
	req.type   = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	req.memory = V4L2_MEMORY_USERPTR;

	if (-1 == xioctl(fd, VIDIOC_REQBUFS, &req)) {
		if (EINVAL == errno) {
			printf("%s does not support "
				"user pointer i/o\n", dev_name);
			exit(EXIT_FAILURE);
		} else {
			errno_exit("VIDIOC_REQBUFS");
		}
	}

	buffers = (buffer*) calloc(4, sizeof(*buffers));

	if (!buffers) {
		printf("Out of memory\n");
		exit(EXIT_FAILURE);
	}

	for (n_buffers = 0; n_buffers < 4; ++n_buffers) {
		buffers[n_buffers].length = buffer_size;
		buffers[n_buffers].start = malloc(buffer_size);

		if (!buffers[n_buffers].start) {
			printf("Out of memory\n");
			exit(EXIT_FAILURE);
		}
	}
}

static void init_device(void)
{
	struct v4l2_capability cap;
	struct v4l2_cropcap cropcap;
	struct v4l2_crop crop;
	unsigned int min;

	if (-1 == xioctl(fd, VIDIOC_QUERYCAP, &cap)) {
		if (EINVAL == errno) {
			printf("%s is no V4L2 device\n",
				dev_name);
			exit(EXIT_FAILURE);
		} else {
			errno_exit("VIDIOC_QUERYCAP");
		}
	}

	if (!(cap.capabilities & V4L2_CAP_VIDEO_CAPTURE)) {
		printf("%s is no video capture device\n",
			dev_name);
		exit(EXIT_FAILURE);
	}

	switch (io) {
	case IO_METHOD_READ:
		if (!(cap.capabilities & V4L2_CAP_READWRITE)) {
			printf("%s does not support read i/o\n",
				dev_name);
			exit(EXIT_FAILURE);
		}
		break;

	case IO_METHOD_MMAP:
	case IO_METHOD_USERPTR:
		if (!(cap.capabilities & V4L2_CAP_STREAMING)) {
			printf("%s does not support streaming i/o\n",
				dev_name);
			exit(EXIT_FAILURE);
		}
		break;
	}


	/* Select video input, video standard and tune here. */


	CLEAR(cropcap);

	cropcap.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

	if (0 == xioctl(fd, VIDIOC_CROPCAP, &cropcap)) {
		crop.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		crop.c = cropcap.defrect; /* reset to default */

		if (-1 == xioctl(fd, VIDIOC_S_CROP, &crop)) {
			switch (errno) {
			case EINVAL:
				/* Cropping not supported. */
				break;
			default:
				/* Errors ignored. */
				break;
			}
		}
	} else {
		/* Errors ignored. */
	}


	CLEAR(fmt);

	fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	if (force_format)
	{
		//printf("Set H264\r\n");
		fmt.fmt.pix.width       = 1920; //640; //replace
		fmt.fmt.pix.height      = 1080; //480; //replace
		fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_YUYV ; //V4L2_PIX_FMT_H264; //replace
		fmt.fmt.pix.field       = V4L2_FIELD_ANY;

		if (-1 == xioctl(fd, VIDIOC_S_FMT, &fmt))
			errno_exit("VIDIOC_S_FMT");

		/* Note VIDIOC_S_FMT may change width and height. */
	} else {
		/* Preserve original settings as set by v4l2-ctl for example */
		if (-1 == xioctl(fd, VIDIOC_G_FMT, &fmt))
			errno_exit("VIDIOC_G_FMT");
	}

	/* Buggy driver paranoia. */
	min = fmt.fmt.pix.width * 2;
	if (fmt.fmt.pix.bytesperline < min)
		fmt.fmt.pix.bytesperline = min;
	min = fmt.fmt.pix.bytesperline * fmt.fmt.pix.height;
	if (fmt.fmt.pix.sizeimage < min)
		fmt.fmt.pix.sizeimage = min;

	switch (io) {
	case IO_METHOD_READ:
		init_read(fmt.fmt.pix.sizeimage);
		break;

	case IO_METHOD_MMAP:
		init_mmap();
		break;

	case IO_METHOD_USERPTR:
		init_userp(fmt.fmt.pix.sizeimage);
		break;
	}
}

static void close_device(void)
{
	if (-1 == close(fd))
		errno_exit("close");

	fd = -1;
}

static void open_device(void)
{
	struct stat st;

	if (-1 == stat(dev_name, &st)) {
		printf("Cannot identify '%s': %d, %s\n",
			dev_name, errno, strerror(errno));
		exit(EXIT_FAILURE);
	}

	if (!S_ISCHR(st.st_mode)) {
		printf("%s is no device\n", dev_name);
		exit(EXIT_FAILURE);
	}

	fd = open(dev_name, O_RDWR /* required */ | O_NONBLOCK, 0);

	if (-1 == fd) {
		printf("Cannot open '%s': %d, %s\n",
			dev_name, errno, strerror(errno));
		exit(EXIT_FAILURE);
	}
}

#endif

#if TU_CONFIG_LINK_TO_LIBUVC == 1

const Uint8* my_uvc_yuyv2rgb(uvc_frame_t *in)
{

  uint8_t *pyuv = NULL;
  uint8_t *prgb = NULL;
  uint8_t *prgb_end = NULL;

  int data_bytes = in->width * in->height * 3;
  Uint8* out = (Uint8*) malloc(data_bytes);
  pyuv = (uint8_t*) in->data;
  prgb = (uint8_t*) out;
  prgb_end = prgb + data_bytes;

  while (prgb < prgb_end)
  {
    IYUYV2RGB_8(pyuv, prgb);
    prgb += 3 * 8;
    pyuv += 2 * 8;
  }

	/*
	// rgb ==> rgba
  Uint8* buf = (Uint8*) malloc(in->width * in->height * 4);
  Uint8* dst = buf;
  Uint8* src = out;
	for (int x = 0; x < in->width; x++)
	{
		for (int y = 0; y < in->height; y++)
		{
			dst[0] = src[0];
			dst[1] = src[1];
			dst[2] = src[2];
			dst[3] = 255;
			dst += 4;
			src += 3;
		}
	}
	free(out);
	return buf;
	*/
	return out;
}

struct error_mgr
{
	struct jpeg_error_mgr super;
	jmp_buf jmp;
};

static void _error_exit(j_common_ptr dinfo) {
	struct error_mgr *myerr = (struct error_mgr *)dinfo->err;
	(*dinfo->err->output_message)(dinfo);
	longjmp(myerr->jmp, 1);
}

const Uint8* my_uvc_mjpeg2rgba(uvc_frame_t *in)
{
	struct jpeg_decompress_struct dinfo;
	struct error_mgr jerr;
	size_t lines_read;

	// for rgb ==> rgba
	Uint8* buf = NULL;
	Uint8* dst = NULL;
	Uint8* src = NULL;
	Uint8* out = NULL;

	dinfo.err = jpeg_std_error(&jerr.super);
	jerr.super.error_exit = _error_exit;

	if (setjmp(jerr.jmp)) 
	{
		goto fail;
	}

	jpeg_create_decompress(&dinfo);
	jpeg_mem_src(&dinfo, (unsigned char*) in->data, in->data_bytes);
	jpeg_read_header(&dinfo, TRUE);

	dinfo.out_color_space = JCS_RGB;
	dinfo.dct_method = JDCT_IFAST;

	jpeg_start_decompress(&dinfo);

	lines_read = 0;
	out = (Uint8*) malloc(in->width * in->height * 3);
	while (dinfo.output_scanline < dinfo.output_height) 
	{
		unsigned char* buffer[1] = { (unsigned char*) out + lines_read * in->width * 3 };
		int num_scanlines;

		num_scanlines = jpeg_read_scanlines(&dinfo, buffer, 1);
		lines_read += num_scanlines;
	}

	jpeg_finish_decompress(&dinfo);
	jpeg_destroy_decompress(&dinfo);

/*
	// rgb ==> rgba
	buf = (Uint8*) malloc(in->width * in->height * 4);
	dst = buf;
	src = out;
	for (int x = 0; x < in->width; x++)
	{
		for (int y = 0; y < in->height; y++)
		{
			dst[0] = 255-src[0];
			dst[1] = 255-src[1];
			dst[2] = 255-src[2];
			dst[3] = 255;
			dst += 4;
			src += 3;
		}
	}
	free(out);
	return buf;*/
	return out;

fail:
	jpeg_destroy_decompress(&dinfo);
	free(out);
	return NULL;
}


void uvc_callback(uvc_frame_t *frame, void* ptr) 
{
//  Uint32 start = tu_timer::get_ticks();
	const Uint8* buf = NULL;
  switch (frame->frame_format)
  {
    case UVC_FRAME_FORMAT_YUYV:
      buf = my_uvc_yuyv2rgb(frame);
      break;

    case  UVC_FRAME_FORMAT_MJPEG:
		buf = my_uvc_mjpeg2rgba(frame);
    	break;

    default:
      assert(0); //return UVC_ERROR_NOT_SUPPORTED;
  }

	if (buf)
	{
		// Call a C++ method:
		bakeinflash::as_camera_object* camobj = (bakeinflash::as_camera_object*) ptr;
		camobj->set_video_data((Uint8*) buf);
	}
//  printf("%d\n", tu_timer::get_ticks() - start);
}
#endif


namespace bakeinflash
{
	//	public static get([index:Number]) : camera
	void	camera_get(const fn_call& fn)
	{
		as_camera* cam = cast_to<as_camera>(fn.this_ptr);
		if (cam)
		{
			fn.result->set_as_object(cam->get());
		}
	}
	
	void	camera_take_image(const fn_call& fn)
	{
		as_camera* cam = cast_to<as_camera>(fn.this_ptr);
		if (fn.nargs >= 2 && cam)
		{
			cam->take_image(cast_to<character>(fn.arg(0).to_object()), fn.arg(1).to_float());
		}
	}

	as_object* camera_init()
	{
		// Create built-in global Camera object.
		as_object*	obj = new as_camera();
		obj->builtin_member("get", camera_get);
		obj->builtin_member("takeImage", camera_take_image);
		return obj;
	}
	
	as_camera::as_camera()
	{
		// I do not whant here the code becuase it froozes app
	}

	as_object* as_camera::get()
	{
		return new as_camera_object();
	}

	as_camera::~as_camera()
	{
#ifdef iiiiiOS
		myCamera* mycam = (myCamera*) m_cam;
		[mycam release];
#endif
	}

	void as_camera::notify_image(bool rc)
	{
#ifdef iiiiiOS
		myCamera* mycam = (myCamera*) m_cam;
		[mycam hide];
#endif		
		bakeinflash::as_value function;
		if (get_member("onImage", &function))
		{
			as_environment env;
			env.push(true);
			call_method(function, &env, this, 1, env.get_top_index());
		}
		
	}	
	
	void as_camera::take_image(character* target, float quality)
	{
		int size = 0;
		const void* jpegdata = NULL;
#ifdef iiiiiOS
		myCamera* mycam = (myCamera*) m_cam;
		[mycam getJpeg :quality :&size : &jpegdata];
#endif
		
		myprintf("jpeg size=%d\n", size);
		
		if (size == 0)
		{
			return;
		}
		
		tu_file fi(tu_file::memory_buffer, size, (void*) jpegdata);
		image::rgb* im = image::read_jpeg(&fi);
		if (im)
		{
			bitmap_info* bi = render::create_bitmap_info(im);
			
			movie_definition* rdef = get_root()->get_movie_definition();
			assert(rdef);
			bitmap_character_def*	jpeg = new bitmap_character_def(rdef, bi);
			
			if (target)
			{
				target->replace_me(jpeg);
			}
		}
		myprintf("notifyTakeImage %d\n", size);
	}
	
	//
	//
	//
	
	as_camera_object::as_camera_object() :
		m_video_data(NULL)
#if TU_CONFIG_LINK_TO_LIBUVC == 1
	,
		m_uvc_dev(NULL),
		m_uvc_devh(NULL),
		m_uvc_ctx(NULL)
#endif
	{
#ifdef iiiiOS
		if (m_cam == NULL)
		{
			myCamera* mycam = [[myCamera alloc] init];
			mycam.m_parent = this;
			m_cam = mycam;
		}
		myCamera* mycam = (myCamera*) m_cam;
		[mycam show];
#else

		open_stream(NULL);

#endif
	}

	as_camera_object::~as_camera_object()
	{
		close_stream();
	}

	// pass video data in video_stream_instance()
	Uint8* as_camera_object::get_video_data()
	{
#if TU_CONFIG_LINK_TO_V4L2 == 1


//		Uint8* zz = (Uint8*) malloc(640*480*3);
//		for (int i=0; i<640*480*3; i++)
//		{
//			zz[i] = tu_random::next_random();
//		}
//		return zz;

		fd_set fds;
		struct timeval tv;
		int r;

		FD_ZERO(&fds);
		FD_SET(fd, &fds);

		/* Timeout. */
		tv.tv_sec = 2;
		tv.tv_usec = 0;

		r = select(fd + 1, &fds, NULL, NULL, &tv);

		if (-1 == r)
		{
			if (EINTR == errno)	return NULL;
			errno_exit("select");
			return NULL;
		}

		if (0 == r)
		{
			printf("select timeout\n");
//			exit(EXIT_FAILURE);
			return NULL;
		}

		Uint8* data = NULL;
		int size = NULL;
		int rc = read_frame(&data, &size);
		if (data && size > 0)
		{
			//printf("video size: %d\n", size);

			Uint8* image = (Uint8*) my_uvc_yuyv2rgb(data, size);
			return image;
		/* EAGAIN - continue select loop. */
		}

		printf("no video\n");
	return NULL;
#else
		tu_autolock locker(m_lock_video);
		Uint8* video_data = m_video_data;
		m_video_data = NULL;
		return video_data;
#endif
	}

	void as_camera_object::set_video_data(Uint8* data)
	{
		tu_autolock locker(m_lock_video);
		if (m_video_data)
		{
			free(m_video_data);
		}
		m_video_data = data;
	}

	void as_camera_object::free_video_data(Uint8* data)
	{
		free(data);
	}

	bool as_camera_object::open_stream(const char* url)
	{
#if TU_CONFIG_LINK_TO_V4L2 == 1

        open_device();
        init_device();
        start_capturing();
        //mainloop();

		return true;
#endif

#if TU_CONFIG_LINK_TO_LIBUVC == 1

		uvc_error_t res;

		res = uvc_init(&m_uvc_ctx, NULL);
		if (res < 0)
		{
			uvc_perror(res, "uvc_init");
			close_stream();
			return false;
		}

		// Locates the first attached UVC device, stores in dev
		// filter devices: vendor_id, product_id, "serial_num" 
	  res = uvc_find_device(m_uvc_ctx, &m_uvc_dev, 0, 0, NULL); 
		if (res < 0)
		{
			// no devices found
			uvc_perror(res, "uvc_find_device"); 
			close_stream();
			return false;
		}

		// Try to open the device: requires exclusive access
		res = uvc_open(m_uvc_dev, &m_uvc_devh);
		if (res < 0)
		{
			// unable to open device 
			uvc_perror(res, "uvc_open");
			close_stream();
			return false;
		}

		// Print out a message containing all the information that libuvc knows about the device
		uvc_print_diag(m_uvc_devh, stdout);

		// Try to negotiate a 640x480 30 fps YUYV stream profile
		uvc_stream_ctrl_t ctrl = {};
		//res = uvc_get_stream_ctrl_format_size(m_uvc_devh, &ctrl, UVC_FRAME_FORMAT_MJPEG, 1920, 1080, 30);
		res = uvc_get_stream_ctrl_format_size(m_uvc_devh, &ctrl, UVC_FRAME_FORMAT_YUYV, 1920, 1080, 30);
		//res = uvc_get_stream_ctrl_format_size(m_uvc_devh, &ctrl, UVC_FRAME_FORMAT_YUYV, 1920, 1080, 5);

		// Print out the result
		uvc_print_stream_ctrl(&ctrl, stdout);

		if (res < 0) 
		{
			// device doesn't provide a matching stream 
			uvc_perror(res, "get_mode");
			close_stream();
			return false;
		}

		// Start the video stream. The library will call user function  cb(frame, (void*) 12345)
		res = uvc_start_streaming(m_uvc_devh, &ctrl, uvc_callback, this, 0);
		if (res < 0)
		{
			uvc_perror(res, "start_streaming"); // unable to start stream 
			close_stream();
			return false;
		} 

		puts("Streaming...");
		uvc_set_ae_mode(m_uvc_devh, 1); // e.g., turn on auto exposure 
		return true;

#else

		return false;

#endif
	}

	void as_camera_object::close_stream()
	{
#if TU_CONFIG_LINK_TO_V4L2 == 1

		stop_capturing();
		uninit_device();
		close_device();

#endif

#if TU_CONFIG_LINK_TO_LIBUVC == 1
		// End the stream. Blocks until last callback is serviced 
		if (m_uvc_devh)
		{
			uvc_stop_streaming(m_uvc_devh);

			// Release our handle on the device 
			uvc_close(m_uvc_devh);
			m_uvc_devh = NULL;
		}

		// Release the device descriptor
		if (m_uvc_dev)
		{
			uvc_unref_device(m_uvc_dev);
			m_uvc_dev = NULL;
		}

		if (m_uvc_ctx)
		{
			uvc_exit(m_uvc_ctx);
			m_uvc_ctx = NULL;
		}
		#endif
	}

	int as_camera_object::get_width() const
	{
#if TU_CONFIG_LINK_TO_LIBUVC == 1
		if (m_uvc_ctx)
		{
			return 1920;
		}
#endif
#if TU_CONFIG_LINK_TO_V4L2 == 1
		return fmt.fmt.pix.width;
#endif
		return 0;
	}

	int as_camera_object::get_height() const
	{
#if TU_CONFIG_LINK_TO_LIBUVC == 1
		if (m_uvc_ctx)
		{
			return 1080;
		}
#endif
#if TU_CONFIG_LINK_TO_V4L2 == 1
		return fmt.fmt.pix.height;
#endif
		return 0;
	}

} // end of bakeinflash namespace

#ifdef iOS

@implementation myCamera
@synthesize m_parent;
@synthesize m_picker;
@synthesize m_image;
@synthesize m_imageData;

- (id)init
{
	if ((self = [super init]))
	{
		//		myprintf("ctor\n");
		UIImagePickerController* imagePickerController = [[UIImagePickerController alloc] init];
		imagePickerController.delegate = self;
		imagePickerController.sourceType = UIImagePickerControllerSourceTypeCamera;
		m_picker = imagePickerController;
		m_image = nil;
	}
	return self;
}

- (void)dealloc
{
	//	myprintf("dtor\n");
	[m_picker.view removeFromSuperview];
	[m_picker release];

	if (m_image)
	{
		[m_image release];
	}
	
	[super dealloc];
}

- (void)getJpeg :(float) quality :(int*) size :(const void**) data
{
	if (m_image == nil)
	{
		*size = 0;
		*data = nil;
	}
	else
	{
		if (m_imageData == nil)
		{
			m_imageData = m_image == nil ? nil : UIImageJPEGRepresentation(m_image, quality);
		}
		*size = m_imageData.length;
		*data = m_imageData.bytes;
	}
}

- (void)imagePickerController:(UIImagePickerController *)picker	didFinishPickingImage:(UIImage *)image editingInfo:(NSDictionary *)editingInfo
{
	if (m_image)
	{
		[m_image release];
	}
	m_image = image;
	[m_image retain];
	m_imageData = nil;
	
	bakeinflash::as_camera* cam = (bakeinflash::as_camera*) m_parent;
	cam->notify_image(true);
}

//- (void)imagePickerController:(UIImagePickerController *)picker didFinishPickingMediaWithInfo:(NSDictionary *)info
//{
//	bakeinflash::as_camera* cam = (bakeinflash::as_camera*) m_parent;
//	cam->notify_image(false);
//}

- (void)imagePickerControllerDidCancel:(UIImagePickerController *)picker
{
	if (m_image)
	{
		[m_image release];
	}
	m_image = nil;
	m_imageData = nil;
	
	bakeinflash::as_camera* cam = (bakeinflash::as_camera*) m_parent;
	cam->notify_image(false);
}

- (void) show
{
	m_picker.view.hidden = NO;
	[m_picker.view removeFromSuperview];
	[s_view addSubview:m_picker.view];
//	[m_picker takePicture];
}

- (void) hide
{
	m_picker.view.hidden = YES;
}



@end

#endif	// iOS
