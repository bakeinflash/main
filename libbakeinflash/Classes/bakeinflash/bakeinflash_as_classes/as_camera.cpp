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
#include "bakeinflash/bakeinflash_as_classes/as_array.h"
#include "base/tu_file.h"
#include "base/tu_random.h"
#include "base/tu_timer.h"
#include "jpeglib/jpeglib.h"
#include <setjmp.h>

#ifdef WIN32
#include <shlobj.h>		// for windows
#include <mfobjects.h>
#include <mfapi.h>
#include <mfidl.h>
#endif

#if TU_CONFIG_LINK_TO_OPENCV == 1
  #include <vector>
  using namespace cv;
 // using namespace cv::face;
  using namespace std;
#endif

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

#define V4L2_BUFFERS 2

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

	req.count = V4L2_BUFFERS;
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

	req.count  = V4L2_BUFFERS;
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

	buffers = (buffer*) calloc(V4L2_BUFFERS, sizeof(*buffers));

	if (!buffers) {
		printf("Out of memory\n");
		exit(EXIT_FAILURE);
	}

	for (n_buffers = 0; n_buffers < V4L2_BUFFERS; ++n_buffers) {
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

#if defined(__APPLE__) && !defined(iOS)
const char* getTmpPictureFileName()
{
  time_t _tm = time(NULL );
  struct tm * curtime = localtime (&_tm);
  
  const char* home = getenv("HOME");
  
  char* timestamp[80];
  strftime((char*) timestamp, 80, "%d%m%Y.%I%M%S", curtime);
  
  static char tmppicturespath[1024];
  snprintf(tmppicturespath, 1024, "%s/Pictures/microscope-%s.png", home, timestamp);
  
  return tmppicturespath;
}

const char* getTmpVideoFileName()
{
  time_t _tm = time(NULL );
  struct tm * curtime = localtime (&_tm);
  
  char* timestamp[80];
  strftime((char*) timestamp, 80, "%d%m%Y.%I%M%S", curtime);
  
  const char* home = getenv("HOME");
  
  static char tmppicturespath[1024];
  snprintf(tmppicturespath, 1024, "%s/Movies/microscope-%s.avi", home, timestamp);
  
  return tmppicturespath;
}

bool getDeviceList(bakeinflash::as_array* devices)
{
  return false;
}

#endif


#ifdef WIN32
const char* getTmpPictureFileName()
{
	CHAR picturespath[MAX_PATH];
	HRESULT result = SHGetFolderPath(NULL, CSIDL_MYPICTURES, NULL, SHGFP_TYPE_CURRENT, picturespath);       
	
	time_t _tm = time(NULL );
	struct tm * curtime = localtime (&_tm);

	char* timestamp[80];
	strftime((char*) timestamp, 80, "%d%m%Y.%I%M%S", curtime);

	static CHAR tmppicturespath[MAX_PATH];
	snprintf(tmppicturespath, MAX_PATH, "%s\\microscope%s.png", picturespath, timestamp[0]);

	return tmppicturespath;
}

const char* getTmpVideoFileName()
{
	CHAR picturespath[MAX_PATH];
	HRESULT result = SHGetFolderPath(NULL, CSIDL_MYVIDEO, NULL, SHGFP_TYPE_CURRENT, picturespath);       
	
	time_t _tm = time(NULL );
	struct tm * curtime = localtime (&_tm);

	char* timestamp[80];
	strftime((char*) timestamp, 80, "%d%m%Y_%I%M%S", curtime);

	static CHAR tmppicturespath[MAX_PATH];
	snprintf(tmppicturespath, MAX_PATH, "%s\\microscope_%s.avi", picturespath, timestamp[0]);

	return tmppicturespath;
}

bool getDeviceList(bakeinflash::as_array* devices)
{
	IMFActivate **ppDevices;
	UINT32      count;

	IMFAttributes *pAttributes = NULL;

	HRESULT hr = MFCreateAttributes(&pAttributes, 1);
	if (FAILED(hr))
	{
		return false;
	}

	// Ask for source type = video capture devices
	hr = pAttributes->SetGUID(MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE,	MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE_VIDCAP_GUID);
	if (FAILED(hr))
	{
		return false;
	}

	// Enumerate devices.
	hr = MFEnumDeviceSources(pAttributes, &ppDevices, &count);
	if (FAILED(hr))
	{
		return false;
	}


	// Display a list of the devices.
	for (DWORD i = 0; i < count; i++)
	{
		WCHAR *szFriendlyName = NULL;
		UINT32 cchName;

		hr = ppDevices[i]->GetAllocatedString(MF_DEVSOURCE_ATTRIBUTE_FRIENDLY_NAME,	&szFriendlyName, &cchName);
		if (FAILED(hr))
		{
			break;
		}

		tu_string s;
		for (int i = 0; i < wcslen(szFriendlyName); i++)
		{
			s.append_wide_char((Uint16) szFriendlyName[i]);
		}
		devices->push(s.c_str());

		CoTaskMemFree(szFriendlyName);
	}
	return true;
}

#endif

namespace bakeinflash
{

	// it is running in decoder thread
	static void webcam_streamer(void* arg)
	{
		as_camera_object* webcam = (as_camera_object*) arg;
		webcam->run();
	}


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

	void	camera_object_take_image(const fn_call& fn)
	{
		as_camera_object* cam = cast_to<as_camera_object>(fn.this_ptr);
		if (cam)
		{
			fn.result->set_tu_string(cam->takeImage());
		}
	}

	void	camera_object_video_click(const fn_call& fn)
	{
		as_camera_object* cam = cast_to<as_camera_object>(fn.this_ptr);
		if (cam)
		{
			fn.result->set_tu_string(cam->videoClick());
		}
	}

	void	camera_object_face_recognize(const fn_call& fn)
	{
		as_camera_object* cam = cast_to<as_camera_object>(fn.this_ptr);
		if (cam)
		{
			fn.result->set_tu_string(cam->faceRecognize());
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
#ifdef iOS
		myCamera* mycam = (myCamera*) m_cam;
		[mycam release];
#endif
	}

	void as_camera::notify_image(bool rc)
	{
#ifdef iOS
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
#ifdef iOS
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
		m_video_data(NULL),
		m_go(false),
		m_width(640), //1920),
		m_height(480), //1080),
		m_hue(128),
		m_contrast(128),
		m_brightness(128),
		m_saturation(128),
		m_timeStamp(1),		// date & time
		m_fps(0)
	{
#ifdef iOS
		if (m_cam == NULL)
		{
			myCamera* mycam = [[myCamera alloc] init];
			mycam.m_parent = this;
			m_cam = mycam;
		}
		myCamera* mycam = (myCamera*) m_cam;
		[mycam show];
#else

		builtin_member("takeImage", camera_object_take_image);
		builtin_member("videoClick", camera_object_video_click);

		builtin_member("faceRecognize", camera_object_face_recognize);


		open_stream(NULL);

#endif
	}

	as_camera_object::~as_camera_object()
	{
		close_stream();
	}

	// pass video data in video_stream_instance()
	Uint8* as_camera_object::get_video_data(int* w, int* h)
	{
		tu_autolock locker(m_lock_video);
		Uint8* video_data = m_video_data;
		m_video_data = NULL;
		if (w) *w = get_width();
		if (h) *h = get_height();
		return video_data;
	}

	void as_camera_object::set_video_data(Uint8* data, int w, int h)
	{
		tu_autolock locker(m_lock_video);
		if (m_video_data)
		{
			free_video_data(m_video_data);
		}
		m_video_data = data;
		m_width = w;
		m_height = h;
	}

	void as_camera_object::free_video_data(Uint8* data)
	{
		free(data);
	}

	video_pixel_format::code as_camera_object::get_pixel_format()
	{
		//		return video_pixel_format::YUYV; 
		return video_pixel_format::BGR; 
	}

	bool as_camera_object::open_stream(const char* url)
	{
#if TU_CONFIG_LINK_TO_OPENCV == 1
		bool rc = m_cap.open(0);
		if (m_cap.isOpened() == false) 
		{
			myprintf("cannot open webcam\n");
			return false;
		}

		// default values
		m_cap.set(CV_CAP_PROP_FRAME_WIDTH, m_width);
		m_cap.set(CV_CAP_PROP_FRAME_HEIGHT, m_height);

	//	m_cap.set(CV_CAP_PROP_BRIGHTNESS, m_contrast);
	//	m_cap.set(CV_CAP_PROP_CONTRAST, m_contrast);
	//	m_cap.set(CV_CAP_PROP_SATURATION, m_saturation);
	//	m_cap.set(CV_CAP_PROP_HUE, m_hue);

		// get devices
		as_array* devices = new as_array();
		rc = getDeviceList(devices);
		set_member("devices", devices);
		set_member("width" , m_width);
		set_member("height" , m_height);

		cvInitFont(&m_font, FONT_HERSHEY_PLAIN, 0.5, 0.5);

		m_thread = new tu_thread(webcam_streamer, this);
		return true;
#endif

#if TU_CONFIG_LINK_TO_V4L2 == 1
		open_device();
		init_device();
		start_capturing();
		m_thread = new tu_thread(webcam_streamer, this);
#endif

		return true;
	}

	void as_camera_object::close_stream()
	{
#if TU_CONFIG_LINK_TO_OPENCV == 1
		if (m_thread != NULL)
		{
			m_go = false;
			m_thread->wait();
		}

		m_cap.release();

		set_video_data(NULL, 0, 0);
#endif 

#if TU_CONFIG_LINK_TO_V4L2 == 1
		stop_capturing();
		uninit_device();
		close_device();

#endif
	}

	int as_camera_object::get_width() const
	{
#if TU_CONFIG_LINK_TO_V4L2 == 1
		return fmt.fmt.pix.width;
#endif
		return m_width;
	}

	int as_camera_object::get_height() const
	{
#if TU_CONFIG_LINK_TO_V4L2 == 1
		return fmt.fmt.pix.height;
#endif
		return m_height;
	}


	// new thread !!!!
	void as_camera_object::run()
	{
#if TU_CONFIG_LINK_TO_OPENCV == 1
		m_go = true;

		int frames = 0;
		Uint32	prev_frames_ticks = tu_timer::get_ticks();
		while (m_go)
		{
			Uint32 ticks = tu_timer::get_ticks();

			// calc FPS
			frames++;
			if (ticks - prev_frames_ticks >= 1000)
			{
				m_fps = (int)(frames * 1000.0f / (ticks - prev_frames_ticks));
				prev_frames_ticks = ticks;
				frames = 0;
			//	printf("fps=%d\n", m_fps);
			}

			m_mutex.lock();
			bool rc = m_cap.read(m_frame); // the image data will be stored in the BGR format.
			if (rc)
			{
				// apply hue
					cvtColor(m_frame, m_frame, COLOR_BGR2HSV);
					for (int i = 0; i < m_frame.rows; i++)   
					{
						for (int j = 0; j < m_frame.cols; j++)
						{
							Vec3b& hsv = m_frame.at<Vec3b>(i, j);
							hsv.val[0] = iclamp(hsv.val[0] + (m_hue - 128), 0, 255);
							hsv.val[1] = iclamp(hsv.val[1] + (m_saturation - 128), 0, 255);
							hsv.val[2] = iclamp(hsv.val[2] + (m_brightness - 128), 0, 255);

							// contrast, .SV
							hsv.val[1] = iclamp(hsv.val[1] + (m_contrast - 128), 0, 255);
							hsv.val[2] = iclamp(hsv.val[2] + (m_contrast - 128), 0, 255);

						}
					}
					cvtColor(m_frame, m_frame, COLOR_HSV2BGR);

				if (m_avi.isOpened())
				{
					m_avi.write(m_frame);
				}

				// time
				time_t _tm = time(NULL );
				struct tm * curtime = localtime (&_tm);
				char* timestamp[80];
				strftime((char*) timestamp, 80, m_timeStamp == 0 ? "%d/%m/%Y" : "%d/%m/%Y %I:%M:%S", curtime);

				IplImage* img = new IplImage(m_frame);
				cvPutText(img, (const char*) timestamp, cvPoint(10, m_height - 15), &m_font, Scalar::all(255));
				delete img;

				m_mutex.unlock();

				int size = m_frame.dataend - m_frame.datastart;
				assert(size == m_frame.rows * m_frame.cols * 3);		// BGR
				Uint8* data = (Uint8*) malloc(size);
				memcpy(data, m_frame.data, size);

				set_video_data(data, m_frame.cols, m_frame.rows);
			}
			else
			{
				m_mutex.unlock();
				tu_timer::sleep(10);
			}
		}
#endif

#if TU_CONFIG_LINK_TO_V4L2 == 1
		fd_set fds;
		struct timeval tv;
		int r;

		FD_ZERO(&fds);
		FD_SET(fd, &fds);

		// Timeout.
		tv.tv_sec = 2;
		tv.tv_usec = 0;

		Uint32 t = tu_timer::get_ticks();
		r = select(fd + 1, &fds, NULL, NULL, &tv);
		//printf("read time %d\n", tu_timer::get_ticks() -t);

		if (-1 == r)
		{
			//				if (EINTR == errno)	return NULL;
			//				errno_exit("select");
			//				return NULL;
			printf("webcam: select error\n");
			tu_timer::sleep(100);
			continue;
		}

		if (0 == r)
		{
			printf("webcam: select timeout\n");
			tu_timer::sleep(100);
			continue;
		}

		Uint8* data = NULL;
		int size = NULL;
		int rc = read_frame(&data, &size);
		if (rc == 1 && data && size > 0)
		{
			//printf("got frame\n");
			set_video_data(data);
		}
		else
		{
			tu_timer::sleep(1);
		}
#endif
	}

	bool	as_camera_object::get_member(const tu_string& name, as_value* val)
	{
#if TU_CONFIG_LINK_TO_OPENCV == 1
		if (name == "BRIGHTNESS")
		{
			val->set_double(m_brightness);
			return true;
		}
		else
		if (name == "CONTRAST")
		{
			val->set_double(m_contrast);
			return true;
		}
		else
		if (name == "SATURATION")
		{
			val->set_double(m_saturation);
			return true;
		}
		else
		if (name == "HUE")
		{
			val->set_double(m_hue);
			return true;
		}
#endif
		return as_object::get_member(name, val);
	}

	bool as_camera_object::set_member(const tu_string& name, const as_value& val)
	{ 
#if TU_CONFIG_LINK_TO_OPENCV == 1
		if (name == "size")
		{
			as_object* obj = val.to_object();
			if (obj)
			{
				as_value w;
				obj->get_member("w", &w);
				as_value h;
				obj->get_member("h", &h);

				m_mutex.lock();
				m_cap.set(CV_CAP_PROP_FRAME_WIDTH, w.to_number());
				m_cap.set(CV_CAP_PROP_FRAME_HEIGHT, h.to_number());
				int new_width = (int) m_cap.get(CV_CAP_PROP_FRAME_WIDTH);
				int new_height = (int) m_cap.get(CV_CAP_PROP_FRAME_HEIGHT);
				m_mutex.unlock();

				// refresh
				set_member("width" , new_width);
				set_member("height" , new_height);

			}
			return true;
		}
		else
		if (name == "BRIGHTNESS")
		{
			m_brightness = int(val.to_number());
			return true; //m_cap.set(CV_CAP_PROP_BRIGHTNESS, m_brightness);
		}
		else
		if (name == "CONTRAST")
		{
			m_contrast = int(val.to_number());
			return true; //m_cap.set(CV_CAP_PROP_CONTRAST, m_contrast);
		}
		else
		if (name == "SATURATION")
		{
			m_saturation = int(val.to_number());
			return true; //m_cap.set(CV_CAP_PROP_SATURATION, m_saturation);
		}
		else
		if (name == "HUE")
		{
			m_hue = int(val.to_number());
			return true; //	m_cap.set(CV_CAP_PROP_HUE, m_hue);
		}
		else
		if (name == "timeStamp")
		{
			 m_timeStamp = val.to_tu_string() == "Date" ? 0 : 1;
			return as_object::set_member(name, val);
		}
#endif
		return as_object::set_member(name, val);
	}


	tu_string as_camera_object::takeImage()
	{ 
		bool rc = false;
		const char* finame = NULL;
#if TU_CONFIG_LINK_TO_OPENCV == 1
		finame = getTmpPictureFileName();
    std::vector<int> compression_params;
		compression_params.push_back(CV_IMWRITE_PNG_COMPRESSION);
		compression_params.push_back(9);

		m_mutex.lock();
    rc = imwrite(finame, m_frame, compression_params);
		m_mutex.unlock();
#endif
		return rc ? finame : "";
	}

	tu_string as_camera_object::faceRecognize()
	{ 
#if 0
   // These vectors hold the images and corresponding labels:
    vector<Mat> images;
    vector<int> labels;

    // Read in the data (fails if no valid input filename is given, but you'll get an error message):
    try {
//        read_csv(fn_csv, images, labels);
    } catch (cv::Exception& e) {
  //      cerr << "Error opening file \"" << fn_csv << "\". Reason: " << e.msg << endl;
        // nothing more we can do
        exit(1);
    }
    // Get the height from the first image. We'll need this
    // later in code to reshape the images to their original
    // size AND we need to reshape incoming faces to this size:

    // Create a FaceRecognizer and train it on the given images:
    Ptr<FaceRecognizer> model = createFisherFaceRecognizer();

		Mat img = imread("c:\\users\\vitaly\\Pictures\\trainimage1.png", 0);
    int im_width = img.cols;
    int im_height =img.rows;

		images.push_back(img);
		labels.push_back(1); //atoi(classlabel.c_str()));
		images.push_back(img);
		labels.push_back(2); //atoi(classlabel.c_str()));
		model->train(images, labels);
    // That's it for learning the Face Recognition model. You now
    // need to create the classifier for the task of Face Detection.
    // We are going to use the haar cascade you have specified in the
    // command line arguments:
    //
    CascadeClassifier haar_cascade;
	//	string fn_haar = "C:\\bakeinflash\\opencv\\sources\\data\\haarcascades\\haarcascade_frontalface_default.xml";
	//	string fn_haar = "C:\\bakeinflash\\opencv\\sources\\data\\haarcascades\\haarcascade_eye.xml";// глаз
		string fn_haar = "C:\\bakeinflash\\opencv\\sources\\data\\haarcascades\\haarcascade_frontalface_alt.xml";			// нашлел !!!
    bool rc = haar_cascade.load(fn_haar);
		assert(rc);
    // Get a handle to the Video device:
		 int deviceId = 0;
    VideoCapture cap(deviceId);
    // Check if we can use this device at all:
    if(!cap.isOpened()) {
      //  cerr << "Capture Device ID " << deviceId << "cannot be opened." << endl;
        return -1;
    }
    // Holds the current frame from the Video device:
    Mat frame;
    for(;;) {
        cap >> frame;
        // Clone the current frame:
        Mat original = frame.clone();
        // Convert the current frame to grayscale:
        Mat gray;
        cvtColor(original, gray, CV_BGR2GRAY);
        // Find the faces in the frame:
        vector< Rect_<int> > faces;
        haar_cascade.detectMultiScale(gray, faces);
        // At this point you have the position of the faces in
        // faces. Now we'll get the faces, make a prediction and
        // annotate it in the video. Cool or what?
        for(int i = 0; i < faces.size(); i++)
				{
            // Process face by face:
            Rect face_i = faces[i];
            // Crop the face from the image. So simple with OpenCV C++:
            Mat face = gray(face_i);
            // Resizing the face is necessary for Eigenfaces and Fisherfaces. You can easily
            // verify this, by reading through the face recognition tutorial coming with OpenCV.
            // Resizing IS NOT NEEDED for Local Binary Patterns Histograms, so preparing the
            // input data really depends on the algorithm used.
            //
            // I strongly encourage you to play around with the algorithms. See which work best
            // in your scenario, LBPH should always be a contender for robust face recognition.
            //
            // Since I am showing the Fisherfaces algorithm here, I also show how to resize the
            // face you have just found:
            Mat face_resized;
            cv::resize(face, face_resized, Size(im_width, im_height), 1.0, 1.0, INTER_CUBIC);
            // Now perform the prediction, see how easy that is:
            int prediction = model->predict(face_resized);
            // And finally write all we've found out to the original image!
            // First of all draw a green rectangle around the detected face:
            rectangle(original, face_i, CV_RGB(0, 255,0), 1);
            // Create the text we will annotate the box with:
            string box_text = format("Prediction = %d", prediction);
            // Calculate the position for annotated text (make sure we don't
            // put illegal values in there):
            int pos_x = std::max(face_i.tl().x - 10, 0);
            int pos_y = std::max(face_i.tl().y - 10, 0);
            // And now put it into the image:
            putText(original, box_text, Point(pos_x, pos_y), FONT_HERSHEY_PLAIN, 1.0, CV_RGB(0,255,0), 2.0);
        }
        // Show the result:
        imshow("face_recognizer", original);
        // And display it:
        char key = (char) waitKey(20);
        // Exit this loop on escape:
        if(key == 27)
            break;
		}

#endif
		return "";
	}

	tu_string as_camera_object::videoClick()
	{ 
		tu_string finame;
#if TU_CONFIG_LINK_TO_OPENCV == 1
		m_mutex.lock();
		if (m_avi.isOpened())
		{
			m_avi.release();
			finame = "stoped";
		}
		else
		{
			finame = getTmpVideoFileName();
			Size S = Size((int) m_cap.get(CV_CAP_PROP_FRAME_WIDTH),  (int) m_cap.get(CV_CAP_PROP_FRAME_HEIGHT));   // Acquire input size

			// int ex = static_cast<int>(m_cap.get(CV_CAP_PROP_FOURCC));     // Get Codec Type- Int form
			int ex = CV_FOURCC('M','J', 'P', 'G') ;		// works!

			bool rc = m_avi.open(finame.c_str(), ex, m_fps > 0 ? m_fps : 5, S, true);
			if (rc == false)
			{
//				printf("no codec to write video\n");
				finame = "failed";
			}
		}
		m_mutex.unlock();
#endif
		return finame;
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
