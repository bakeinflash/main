//// bakeinflash_render_handler_ogles.cpp	-- Willem Kokke <willem@mindparity.com> 2003

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

// OpenGL ES based video handler for mobile units

#include "base/tu_config.h"
#include "base/tu_types.h"
#include "bakeinflash_cursor.h"
#include "bakeinflash_log.h"

//#define DEBUG_BITMAPS

#if TU_USE_OGLES == 1
#ifdef iOS
#undef TU_USE_SDL
#endif

#include "base/tu_config.h"

#include "bakeinflash/bakeinflash.h"
#include "bakeinflash/bakeinflash_types.h"
#include "base/image.h"
#include "base/utility.h"
#include "base/png_helper.h"
#include "bakeinflash_render_handler_ogles.h"

#include <string.h>	// for memset()

#if (defined(WIN32) || defined(__GNUC__)) && !defined(iOS) && !defined(ANDROID) && !defined(TARGET_OS_MAC)

//#include <GL/glut.h>

typedef void (APIENTRY* PFNGLDELETEPROGRAMPROC) (GLuint program);
PFNGLDELETEPROGRAMPROC glDeleteProgram = 0;

typedef void (APIENTRY* PFNGLDELETESHADERPROC) (GLuint shader);
PFNGLDELETESHADERPROC glDeleteShader = 0;

typedef GLuint (APIENTRY* PFNGLCREATESHADERPROC) (GLenum type);
PFNGLCREATESHADERPROC glCreateShader = 0;

#if TARGET_OS_MAC
typedef void (APIENTRY* PFNGLSHADERSOURCEPROC) (unsigned int, int, const char *const *, const int *);
PFNGLSHADERSOURCEPROC glShaderSource = 0;
#else
typedef void (APIENTRY* PFNGLSHADERSOURCEPROC) (GLuint shader, GLsizei count, const GLchar *const*string, const GLint *length);
PFNGLSHADERSOURCEPROC glShaderSource = 0;
#endif

typedef void (APIENTRY* PFNGLCOMPILESHADERPROC) (GLuint shader);
PFNGLCOMPILESHADERPROC glCompileShader = 0;

typedef void (APIENTRY* PFNGLGETSHADERIVPROC) (GLuint shader, GLenum pname, GLint *params);
PFNGLGETSHADERIVPROC glGetShaderiv = 0;

typedef void (APIENTRY* PFNGLGETSHADERINFOLOGPROC) (GLuint shader, GLsizei bufSize, GLsizei *length, GLchar *infoLog);
PFNGLGETSHADERINFOLOGPROC glGetShaderInfoLog = 0;

typedef GLuint (APIENTRY* PFNGLCREATEPROGRAMPROC) (void);
PFNGLCREATEPROGRAMPROC glCreateProgram = 0;

typedef void (APIENTRY* PFNGLATTACHSHADERPROC) (GLuint program, GLuint shader);
PFNGLATTACHSHADERPROC glAttachShader = 0;

typedef void (APIENTRY* PFNGLLINKPROGRAMPROC) (GLuint program);
PFNGLLINKPROGRAMPROC glLinkProgram = 0;

typedef void (APIENTRY* PFNGLGETPROGRAMIVPROC) (GLuint program, GLenum pname, GLint *params);
PFNGLGETPROGRAMIVPROC glGetProgramiv = 0;

typedef void (APIENTRY* PFNGLGETPROGRAMINFOLOGPROC) (GLuint program, GLsizei bufSize, GLsizei *length, GLchar *infoLog);
PFNGLGETPROGRAMINFOLOGPROC glGetProgramInfoLog = 0;

typedef void (APIENTRY* PFNGLVALIDATEPROGRAMPROC) (GLuint program);
PFNGLVALIDATEPROGRAMPROC glValidateProgram = 0;

typedef GLint (APIENTRY* PFNGLGETUNIFORMLOCATIONPROC) (GLuint program, const GLchar *name);
PFNGLGETUNIFORMLOCATIONPROC glGetUniformLocation = 0;

typedef void (APIENTRY* PFNGLUNIFORM1FPROC) (GLint location, GLfloat v0);
PFNGLUNIFORM1FPROC glUniform1f = 0;

typedef void (APIENTRY* PFNGLUNIFORM2FPROC) (GLint location, GLfloat v0, GLfloat v1);
PFNGLUNIFORM2FPROC glUniform2f = 0;

typedef void (APIENTRY* PFNGLUNIFORM3FPROC) (GLint location, GLfloat v0, GLfloat v1, GLfloat v2);
PFNGLUNIFORM3FPROC glUniform3f = 0;

typedef void (APIENTRY* PFNGLUNIFORM1IPROC) (GLint location, GLint v0);
PFNGLUNIFORM1IPROC glUniform1i = 0;

typedef void (APIENTRY* PFNGLUNIFORM1IVPROC) (GLint  location,  GLsizei  count,  const GLint * value);
PFNGLUNIFORM1IVPROC glUniform1iv = 0;

typedef void (APIENTRY* PFNGLUNIFORM1FVPROC) (GLint  location,  GLsizei  count,  const GLfloat * value);
PFNGLUNIFORM1FVPROC glUniform1fv = 0;

typedef void (APIENTRY* PFNGLUSEPROGRAMPROC) (GLuint program);
PFNGLUSEPROGRAMPROC glUseProgram = 0;

typedef void (APIENTRY* PFNGLUNIFORMMATRIX2FV) (GLint  location,  GLsizei  count,  GLboolean  transpose,  const GLfloat * value);
PFNGLUNIFORMMATRIX2FV glUniformMatrix2fv = 0;

typedef void (APIENTRY* PFNGLUNIFORMMATRIX4FV) (GLint  location,  GLsizei  count,  GLboolean  transpose,  const GLfloat * value);
PFNGLUNIFORMMATRIX4FV glUniformMatrix4fv = 0;

typedef void (APIENTRY* PFNGLDETACHSHADER) (GLuint  program,  GLuint  shader);
PFNGLDETACHSHADER glDetachShader = 0;

typedef void (APIENTRY* PFNGLBINDATTRIBLOCATION) (GLuint  program,  GLuint  index,  const GLchar * name);
PFNGLBINDATTRIBLOCATION glBindAttribLocation = 0;

typedef void (APIENTRY* PFNGLUNIFORM2FV) (GLint  location,  GLsizei  count,  const GLfloat * value);
PFNGLUNIFORM2FV glUniform2fv = 0;
typedef void (APIENTRY* PFNGLUNIFORM3FV) (GLint  location,  GLsizei  count,  const GLfloat * value);
PFNGLUNIFORM3FV glUniform3fv = 0;
typedef void (APIENTRY* PFNGLUNIFORM4FV) (GLint  location,  GLsizei  count,  const GLfloat * value);
PFNGLUNIFORM4FV glUniform4fv = 0;

typedef void (APIENTRY* PFNGLUNIFORM4F) (GLint  location,  GLfloat  v0,  GLfloat  v1,  GLfloat  v2,  GLfloat  v3);
PFNGLUNIFORM4F glUniform4f = 0;

#if defined(WIN32)
//	typedef void (APIENTRY* PFNGLACTIVETEXTURE) (GLenum  texture);
//	PFNGLACTIVETEXTURE glActiveTexture = 0;
#endif
	
typedef void (APIENTRY* PFNGLFRAMEBUFFERTEXTURE2DEXTPROC) (GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level);
PFNGLFRAMEBUFFERTEXTURE2DEXTPROC glFramebufferTexture2DEXT = 0;

typedef void (APIENTRY* PFNGLGLENABLEVERTEXATTRIBARRAY) (GLint index);
PFNGLGLENABLEVERTEXATTRIBARRAY glEnableVertexAttribArray = 0;

typedef void (APIENTRY* PFNGLGLDISABLEVERTEXATTRIBARRAY	) (GLint index);
PFNGLGLDISABLEVERTEXATTRIBARRAY glDisableVertexAttribArray = 0;

typedef void (APIENTRY* PFNGLGLVERTEXATTRIBPOINTER) (GLint indx, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const GLvoid* ptr);
PFNGLGLVERTEXATTRIBPOINTER	glVertexAttribPointer = 0;

typedef GLint (APIENTRY* PFNGLGETATTRIBLOCATION) (GLuint program, const GLchar* name);
PFNGLGETATTRIBLOCATION glGetAttribLocation = 0;

#endif

// Pointers to opengl extension functions.
typedef char GLchar;

//void checkGlError()
//{
//	GLenum err = glGetError();
//	if (err != GL_NO_ERROR) 
//	{
//		printf("OpenGL Error:\n%s\n", gluErrorString(err));
//		bakeinflash::myprintf("OpenGL Error: %d\n", err);
//	}
//}

void checkGlShader(GLuint shader)
{
	GLint status;
	glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
	if (status == GL_FALSE)
	{
		int loglen;
		char logbuffer[1000];
		glGetShaderInfoLog(shader, sizeof(logbuffer), &loglen, logbuffer);
		bakeinflash::myprintf("Shader Compile Error:\n%.*s", loglen, logbuffer);
	} 
	else
	{
		int loglen;
		char logbuffer[1000];
		glGetShaderInfoLog(shader, sizeof(logbuffer), &loglen, logbuffer);
		if (loglen > 0)
		{
			bakeinflash::myprintf("OpenGL Shader Compile:\n%.*s", loglen, logbuffer);
		}
	}
}

void checkGlProgram(GLuint prog)
{
	GLint status;
	glGetProgramiv(prog, GL_LINK_STATUS, &status);
	if (status == GL_FALSE)
	{
		int loglen;
		char logbuffer[1000];
		glGetProgramInfoLog(prog, sizeof(logbuffer), &loglen, logbuffer);
		bakeinflash::myprintf("OpenGL Program Linker Error:\n%.*s", loglen, logbuffer);
	}
	else
	{
		int loglen;
		char logbuffer[1000];
		glGetProgramInfoLog(prog, sizeof(logbuffer), &loglen, logbuffer);
		if (loglen > 0)
		{
			bakeinflash::myprintf("OpenGL Program Link:\n%.*s", loglen, logbuffer);
		}
		glValidateProgram(prog);
		glGetProgramInfoLog(prog, sizeof(logbuffer), &loglen, logbuffer);
		if (loglen > 0) 
		{
			bakeinflash::myprintf("OpenGL Program Validation results:\n%.*s", loglen, logbuffer);
		}
	}
}

/*
// gluPerspective ==> glFrustum
void gluPerspective(float fov, float aspect, float znear, float zfar)
{
	//	gluPerspective(fov, aspect, nnear, ffar) ==> glFrustum(left, right, bottom, top, nnear, ffar);
	//	fov * 0.5 = arctan ((top-bottom)*0.5 / near)
	//	Since bottom == -top for the symmetrical projection that gluPerspective() produces, then:
	//	top = tan(fov * 0.5) * near
	//	bottom = -top
	//	Note: fov must be in radians for the above formulae to work with the C math library. 
	//	If you have comnputer your fov in degrees (as in the call to gluPerspective()), 
	//	then calculate top as follows:
	//	top = tan(fov*3.14159/360.0) * near
	//	The left and right parameters are simply functions of the top, bottom, and aspect:
	//	left = aspect * bottom
	//	right = aspect * top

	float top = tan(fov * 3.141592f / 360.0f) * znear;
	float bottom = -top;
	float left = aspect* bottom;
	float right = aspect * top;
	glFrustum(left, right, bottom, top, znear, zfar);
}

// used by gluLookAt
void normalize(float v[3])
{
	float r = sqrt(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]);
	if (r == 0.0f) return;
	v[0] /= r; v[1] /= r; v[2] /= r;
}

// used by gluLookAt
void cross(float v1[3], float v2[3], float result[3])
{
	result[0] = v1[1] * v2[2] - v1[2] * v2[1];
	result[1] = v1[2] * v2[0] - v1[0] * v2[2];
	result[2] = v1[0] * v2[1] - v1[1] * v2[0];
}

// We do not want to use GLU library
void gluLookAt(float eyex, float eyey, float eyez, float centerx, float centery,
	float centerz, float upx, float upy, float upz)
{
	float forward[3];
	forward[0] = centerx - eyex;
	forward[1] = centery - eyey;
	forward[2] = centerz - eyez;

	float up[3];
	up[0] = upx;
	up[1] = upy;
	up[2] = upz;

	normalize(forward);

	// Side = forward x up
	float side[3];
	cross(forward, up, side);
	normalize(side);

	// Recompute up as: up = side x forward
	cross(side, forward, up);

	GLfloat m[4][4];

	// make identity
	memset(&m[0], 0, sizeof(m));
	for (int i = 0; i < 4; i++)
	{
		m[i][i] = 1;
	}

	m[0][0] = side[0];
	m[1][0] = side[1];
	m[2][0] = side[2];

	m[0][1] = up[0];
	m[1][1] = up[1];
	m[2][1] = up[2];

	m[0][2] = -forward[0];
	m[1][2] = -forward[1];
	m[2][2] = -forward[2];

	glMultMatrixf(&m[0][0]);
	glTranslated(-eyex, -eyey, -eyez);
}
*/

namespace render_handler_ogles
{

// bitmap_info_ogl declaration
struct bitmap_info_ogl : public bakeinflash::bitmap_info
{
	unsigned int	m_texture_id;
	int m_width;
	int m_height;
	image::image_base* m_suspended_image;
	bool m_keep_alive_source_image;
	float m_xratio;
	float m_yratio;
	int m_bpp;

	bitmap_info_ogl();
	bitmap_info_ogl(image::image_base* im);

	virtual float get_xratio() const { return m_xratio; }
	virtual float get_yratio() const { return m_yratio; }
	virtual void set_xratio(float val) { m_xratio = val; }
	virtual void set_yratio(float val) { m_yratio = val; }

	virtual void upload();
	virtual void update(const Uint8* buf, int width, int height);
	virtual void save(const char* filename)
	{
		// bitmap grubber
#if TU_CONFIG_LINK_TO_LIBPNG
			if (get_data())
			{
				png_helper::write_rgba(filename, get_data(), m_width, m_height, get_bpp());
			}
#endif
	}

	void keep_alive_source_image(bool yes)
	{
		m_keep_alive_source_image = yes;
	}

	// get byte per pixel
	virtual int get_bpp() const
	{
		return m_bpp;
	}

	virtual unsigned char* get_data() const
	{
		if (m_suspended_image)
		{
			return m_suspended_image->m_data;
		}
		return NULL;
	}

	virtual void activate()
	{
		assert(m_texture_id > 0);
		glEnable(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D, m_texture_id);
	}

	~bitmap_info_ogl()
	{
#ifdef DEBUG_BITMAPS
		myprintf("delete bitmap tex=%d, %dx%d\n", m_texture_id, get_width(), get_height());
#endif
		if (m_texture_id > 0)
		{
			glDeleteTextures(1, (GLuint*) &m_texture_id);
			m_texture_id = 0;	// for debuging
		}

		if (m_keep_alive_source_image == false)
		{
			delete m_suspended_image;
		}
	}
		
	virtual int get_width() const { return m_width; }
	virtual int get_height() const { return m_height; }

};

// hack
float	m_display_width = 0;
float	m_display_height = 0;


struct video_handler_ogles : public bakeinflash::video_handler
{
	GLuint m_texture;
	float m_scoord;
	float m_tcoord;
	bakeinflash::rgba m_background_color;
	shader m_shader;

	video_handler_ogles():
		m_texture(0),
		m_scoord(0),
		m_tcoord(0),
		m_background_color(0,0,0,0)	// current background color
	{
	}

	~video_handler_ogles()
	{
		glDeleteTextures(1, &m_texture);
	}


	void display(bakeinflash::video_pixel_format::code pixel_format, Uint8* data, int width, int height, const bakeinflash::matrix* m, const bakeinflash::rect* bounds, const bakeinflash::rgba& color)
	{

		if (m_shader.is_ready() == false)
		{
			const char *vs =
			"attribute vec2 vCoord;" 
			"attribute vec2 tCoord;"
			"uniform vec3 xMat[2];"	
			"uniform vec4 uColor;"
			"varying vec2 texCoord;"
			"void main()"
			"{"
			"	texCoord = tCoord;"
			" float x = xMat[0][0] * vCoord.x + xMat[0][1] * vCoord.y + xMat[0][2];"
			" float y = xMat[1][0] * vCoord.x + xMat[1][1] * vCoord.y + xMat[1][2];"
			"	gl_Position = vec4(x, -y, 0.0, 1.0);"
			"}";

			const char *fs =
#if defined(iOS) || defined(ANDROID)
      "precision lowp float;"
      "precision lowp int;"
      "precision lowp sampler2D;"
#endif
        "varying vec2 texCoord;"
				"uniform sampler2D tex0;"
				"uniform vec4 uColor;"
				"void main(void)"
				"{"
				"	vec4 textureColor = texture2D(tex0, texCoord) * uColor;"
				"	gl_FragColor = textureColor;"
				"}";

			m_shader.compile(vs, fs);
		}


		// this can't be placed in constructor becuase opengl may not be accessible yet
		if (m_texture == 0)
		{
			glEnable(GL_TEXTURE_2D);
			glGenTextures(1, &m_texture);
			glBindTexture(GL_TEXTURE_2D, m_texture);

			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		}

		glBindTexture(GL_TEXTURE_2D, m_texture);
		glEnable(GL_TEXTURE_2D);


		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
//		glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);

		// update texture from video frame
		if (data)
		{
			int p2w = p2(width);
			int p2h = p2(height);
			m_scoord = (float) width / p2w;
			m_tcoord = (float) height / p2h;

			if (m_clear_background && pixel_format == bakeinflash::video_pixel_format::RGBA)
			{
				// set video background color
				// assume left-top pixel of the first frame as background color
				if (m_background_color.m_a == 0)
				{
					m_background_color.m_a = 255;
					m_background_color.m_r = data[2];
					m_background_color.m_g = data[1];
					m_background_color.m_b = data[0];
				}

				// clear video background, input data has BGRA format
				Uint8* p = data;
				for (int y = 0; y < height; y++)
				{
					for (int x = 0; x < width; x++)
					{
						// calculate color distance, dist is in [0..195075]
						int r = m_background_color.m_r - p[2];
						int g = m_background_color.m_g - p[1];
						int b = m_background_color.m_b - p[0];
						float dist = (float) (r * r + g * g + b * b);

						static int s_min_dist = 3 * 64 * 64;	// hack
						Uint8 a = (dist < s_min_dist) ? (Uint8) (255 * (dist / s_min_dist)) : 255;

						p[3] = a;		// set alpha
						p += 4;
					}
				}
			}

			// don't use compressed texture for video, it slows down video
			GLint fmt = GL_RGBA;
      
#if !defined(ANDROID) && !(defined(iOS))
//vv			if (pixel_format == bakeinflash::video_pixel_format::BGR) fmt = GL_BGR;
#endif
			if (pixel_format == bakeinflash::video_pixel_format::RGB) fmt = GL_RGB;

			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, p2w, p2h, 0, fmt, GL_UNSIGNED_BYTE, NULL);
			glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width, height, fmt, GL_UNSIGNED_BYTE, data);
		}

		if (m_scoord == 0.0f && m_scoord == 0.0f)
		{
			// no data
			return;
		}


		GLfloat squareVertices[8]; 
		squareVertices[0] = bounds->m_x_min;
		squareVertices[1] = bounds->m_y_min;
		squareVertices[2] = bounds->m_x_max;
		squareVertices[3] = bounds->m_y_min;
		squareVertices[4] = bounds->m_x_min;
		squareVertices[5] = bounds->m_y_max;
		squareVertices[6] = bounds->m_x_max;
		squareVertices[7] = bounds->m_y_max;

		GLfloat squareTextureCoords[8];
		squareTextureCoords[0] = 0;
		squareTextureCoords[1] = 0;
		squareTextureCoords[2] = m_scoord;
		squareTextureCoords[3] = 0;
		squareTextureCoords[4] = 0;
		squareTextureCoords[5] = m_tcoord;
		squareTextureCoords[6] = m_scoord;
		squareTextureCoords[7] = m_tcoord;


		m_shader.attach();

		GLint position_attrib_index = m_shader.getAttribLocation("vCoord");
		glEnableVertexAttribArray(position_attrib_index);
		glVertexAttribPointer(position_attrib_index, 2, GL_FLOAT, false, 0, squareVertices);

		GLint texcoord_attrib_index = m_shader.getAttribLocation("tCoord");
		glEnableVertexAttribArray(texcoord_attrib_index); // Attribute indexes were received from calls to glGetAttribLocation, or passed into glBindAttribLocation.
		glVertexAttribPointer(texcoord_attrib_index, 2, GL_FLOAT, false, 0, squareTextureCoords); // texcoords_data is a float*, 2 per vertex, representing UV coordinates.

		bakeinflash::matrix mm = *m;
		mm.concatenate_xyscale(2 / m_display_width, 2 / m_display_height);
		mm.m_[0][2] = mm.m_[0][2] / m_display_width * 2 - 1;
		mm.m_[1][2] = mm.m_[1][2] / m_display_height * 2 - 1;

		m_shader.set_uniform3v("xMat", 2, &mm.m_[0][0]);
		
		float cc[4];	cc[0] = color.m_r / 255.0f;	cc[1] = color.m_g / 255.0f;	cc[2] = color.m_b / 255.0f;	cc[3] = color.m_a / 255.0f;
		m_shader.set_uniform4v("uColor", 1, cc);

		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
		m_shader.detach();
        
		glDisableVertexAttribArray(texcoord_attrib_index);
		glDisableVertexAttribArray(position_attrib_index);

		glDisable(GL_TEXTURE_2D);
	}

};

struct render_handler_ogles : public bakeinflash::render_handler
{
	// Some renderer state.

	// Enable/disable antialiasing.
	bool	m_enable_antialias;
	
	// Output size.
	bool m_hasStencilBuffer;
	
	bakeinflash::matrix	m_current_matrix;
	bakeinflash::cxform	m_current_cxform;
	bakeinflash::rgba*	m_current_rgba;

	int m_mask_level;	// nested mask level
	GLuint m_cursor_texture;
	shader m_shader_shape;
	shader m_shader_bitmap;
	shader m_shader_text;
	shader m_shader_line;

	render_handler_ogles(bool hasStencilBuffer) :
		m_enable_antialias(false),
		m_mask_level(0),
		m_hasStencilBuffer(hasStencilBuffer),
		m_current_rgba(NULL),
		m_cursor_texture(0)
	{
	}

	~render_handler_ogles()
	{
		if (m_cursor_texture > 0)
		{
			glDeleteTextures(1, &m_cursor_texture);
		}
	}

	void open()
	{
		// Scan for extensions used by bakeinflash

#if (defined(WIN32) || defined(__GNUC__)) && !defined(iOS) && !defined(ANDROID) && !defined(TARGET_OS_MAC)

		glDeleteProgram = (PFNGLDELETEPROGRAMPROC) SDL_GL_GetProcAddress("glDeleteProgram");
		glDeleteShader = (PFNGLDELETESHADERPROC) SDL_GL_GetProcAddress("glDeleteShader");
		glCreateShader = (PFNGLCREATESHADERPROC) SDL_GL_GetProcAddress("glCreateShader");
		glShaderSource = (PFNGLSHADERSOURCEPROC) SDL_GL_GetProcAddress("glShaderSource");
		glCompileShader = (PFNGLCOMPILESHADERPROC) SDL_GL_GetProcAddress("glCompileShader");
		glGetShaderiv = (PFNGLGETSHADERIVPROC) SDL_GL_GetProcAddress("glGetShaderiv");
		glGetShaderInfoLog = (PFNGLGETSHADERINFOLOGPROC) SDL_GL_GetProcAddress("glGetShaderInfoLog");
		glCreateProgram = (PFNGLCREATEPROGRAMPROC) SDL_GL_GetProcAddress("glCreateProgram");
		glAttachShader = (PFNGLATTACHSHADERPROC) SDL_GL_GetProcAddress("glAttachShader");
		glLinkProgram = (PFNGLLINKPROGRAMPROC) SDL_GL_GetProcAddress("glLinkProgram");
		glGetProgramiv = (PFNGLGETPROGRAMIVPROC) SDL_GL_GetProcAddress("glGetProgramiv");
		glGetProgramInfoLog = (PFNGLGETPROGRAMINFOLOGPROC) SDL_GL_GetProcAddress("glGetProgramInfoLog");
		glValidateProgram = (PFNGLVALIDATEPROGRAMPROC) SDL_GL_GetProcAddress("glValidateProgram");
		glGetUniformLocation = (PFNGLGETUNIFORMLOCATIONPROC) SDL_GL_GetProcAddress("glGetUniformLocation");
		glUseProgram = (PFNGLUSEPROGRAMPROC) SDL_GL_GetProcAddress("glUseProgram");
		glUniform1f = (PFNGLUNIFORM1FPROC) SDL_GL_GetProcAddress("glUniform1f");
		glUniform2f = (PFNGLUNIFORM2FPROC) SDL_GL_GetProcAddress("glUniform2f");
		glUniform3f = (PFNGLUNIFORM3FPROC) SDL_GL_GetProcAddress("glUniform3f");
		glUniform1i = (PFNGLUNIFORM1IPROC) SDL_GL_GetProcAddress("glUniform1i");
		glUniform1iv = (PFNGLUNIFORM1IVPROC) SDL_GL_GetProcAddress("glUniform1iv");
		glUniform1fv = (PFNGLUNIFORM1FVPROC) SDL_GL_GetProcAddress("glUniform1fv");
		glUniformMatrix2fv = (PFNGLUNIFORMMATRIX2FV) SDL_GL_GetProcAddress("glUniformMatrix2fv");
		glUniformMatrix4fv = (PFNGLUNIFORMMATRIX4FV) SDL_GL_GetProcAddress("glUniformMatrix4fv");
		glDetachShader = (PFNGLDETACHSHADER) SDL_GL_GetProcAddress("glDetachShader");
		glBindAttribLocation = (PFNGLBINDATTRIBLOCATION) SDL_GL_GetProcAddress("glBindAttribLocation");
		glUniform2fv = (PFNGLUNIFORM2FV) SDL_GL_GetProcAddress("glUniform2fv");
		glUniform3fv = (PFNGLUNIFORM3FV) SDL_GL_GetProcAddress("glUniform3fv");
		glUniform4fv = (PFNGLUNIFORM4FV) SDL_GL_GetProcAddress("glUniform4fv");
		glUniform4f = (PFNGLUNIFORM4F) SDL_GL_GetProcAddress("glUniform4f");
#if defined(WIN32)
//		glActiveTexture = (PFNGLACTIVETEXTURE) SDL_GL_GetProcAddress("glActiveTexture");
#endif
		glEnableVertexAttribArray = (PFNGLGLENABLEVERTEXATTRIBARRAY) SDL_GL_GetProcAddress("glEnableVertexAttribArray");
		glDisableVertexAttribArray = (PFNGLGLDISABLEVERTEXATTRIBARRAY) SDL_GL_GetProcAddress("glDisableVertexAttribArray");
		glVertexAttribPointer = (PFNGLGLVERTEXATTRIBPOINTER) SDL_GL_GetProcAddress("glVertexAttribPointer");
		glGetAttribLocation = (PFNGLGETATTRIBLOCATION) SDL_GL_GetProcAddress("glGetAttribLocation");


#endif

		// Create texture for cursor
		/*
		glEnable(GL_TEXTURE_2D);
		glGenTextures(1, &m_cursor_texture);
		glBindTexture(GL_TEXTURE_2D, m_cursor_texture);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);	// GL_LINEAR ?
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

		int size = 32;
		int k= TU_ARRAYSIZE(s_cursor);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, size, size, 0, GL_RGBA, GL_UNSIGNED_BYTE, s_cursor);
		*/

		//myprintf("Stencil buffer: %s\n", hasStencilBuffer ? "yes" : "no");


	//result->m_x = m_[0][0] * p.m_x + m_[0][1] * p.m_y + m_[0][2];
	//	result->m_y = m_[1][0] * p.m_x + m_[1][1] * p.m_y + m_[1][2];

		const char *vs =
		"attribute vec2 vCoord;" 
    "attribute vec2 tCoord;"
		"uniform vec3 xMat[2];"	
		"uniform vec4 uColor;"
		"varying vec2 texCoord;"
		"void main()"
		"{"
		"	texCoord = tCoord;"
		" float x = xMat[0][0] * vCoord.x + xMat[0][1] * vCoord.y + xMat[0][2];"
		" float y = xMat[1][0] * vCoord.x + xMat[1][1] * vCoord.y + xMat[1][2];"
		"	gl_Position = vec4(x, -y, 0.0, 1.0);"
		"}";

		const char *fs_shape =
#if defined(iOS) || defined(ANDROID)
			"precision lowp float;"
			"precision lowp int;"
			"precision lowp sampler2D;"
#endif
			"varying vec2 texCoord;"
			"uniform sampler2D tex0;"
			"uniform vec4 uColor;"
			"void main(void)"
			"{"
			"	 gl_FragColor = uColor;"
			"}";

		m_shader_shape.compile(vs, fs_shape);

		const char *fs_text =
#if defined(iOS) || defined(ANDROID)
			"precision lowp float;"
			"precision lowp int;"
			"precision lowp sampler2D;"
#endif
			"varying vec2 texCoord;"
			"uniform sampler2D tex0;"
			"uniform vec4 uColor;"
			"void main(void)"
			"{"
			"	vec4 textureColor = texture2D(tex0, texCoord).aaaa;"
			"	gl_FragColor = textureColor * uColor;"
			"}";

		m_shader_text.compile(vs, fs_text);

		const char *fs_bitmap =
#if defined(iOS) || defined(ANDROID)
			"precision lowp float;"
			"precision lowp int;"
			"precision lowp sampler2D;"
#endif
			"varying vec2 texCoord;"
			"uniform sampler2D tex0;"
			"uniform vec4 uColor;"
			"void main(void)"
			"{"
			"	vec4 textureColor = texture2D(tex0, texCoord) * uColor;"
			"	gl_FragColor = textureColor;"
			"}";

		m_shader_bitmap.compile(vs, fs_bitmap);

		//
		// shader line
		//

		const char *fs_line =
#if defined(iOS) || defined(ANDROID)
			"precision lowp float;"
			"precision lowp int;"
			"precision lowp sampler2D;"
#endif
			"uniform vec4 uColor;"
			"void main(void)"
			"{"
			"	  gl_FragColor = uColor;"
			"}";

		m_shader_line.compile(vs, fs_line);


	}

	void show_cursor(int mouse_x, int mouse_y)
	{
		assert(0);		// fixme
		// real size
		int size = 24;
		glEnable(GL_TEXTURE_2D);

		glEnable(GL_BLEND);
		glBlendFunc(GL_ONE_MINUS_DST_COLOR,GL_ONE_MINUS_SRC_COLOR);

		glBindTexture(GL_TEXTURE_2D, m_cursor_texture);
//vv		glColor4f(1.0f, 1.0f, 1.0f, 1.0f);

		float m_x_min = PIXELS_TO_TWIPS(mouse_x);
		float m_y_min = PIXELS_TO_TWIPS(mouse_y);
		float m_x_max = PIXELS_TO_TWIPS(mouse_x + size);
		float m_y_max = PIXELS_TO_TWIPS(mouse_y + size);

		GLfloat squareVertices[8]; 
		squareVertices[0] = m_x_min; 
		squareVertices[1] = m_y_min; 
		squareVertices[2] = m_x_max; 
		squareVertices[3] = m_y_min; 
		squareVertices[4] = m_x_min;
		squareVertices[5] = m_y_max; 
		squareVertices[6] = m_x_max; 
		squareVertices[7] = m_y_max; 

		GLfloat squareTextureCoords[8];
		squareTextureCoords[0] = 0;
		squareTextureCoords[1] = 0;
		squareTextureCoords[2] = 1; //m_scoord;
		squareTextureCoords[3] = 0;
		squareTextureCoords[4] = 0;
		squareTextureCoords[5] = 1; //m_tcoord;
		squareTextureCoords[6] = 1; //m_scoord;
		squareTextureCoords[7] = 1; //m_tcoord;

		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

		glDisable(GL_TEXTURE_2D);
	}

	void set_antialiased(bool enable)
	{
		m_enable_antialias = enable;
	}

	struct fill_style
	{
		enum mode
		{
			INVALID,
			COLOR,
			BITMAP_WRAP,
			BITMAP_CLAMP,
			LINEAR_GRADIENT,
			RADIAL_GRADIENT,
		};
		mode	m_mode;
		mutable float	m_color[4];
		bakeinflash::bitmap_info*	m_bitmap_info;
		bakeinflash::matrix	m_bitmap_matrix;
		float m_width;	// for line style
		int m_caps_style;
		mutable float	m_pS[4];
		mutable float	m_pT[4];
		
		fill_style() :
			m_mode(INVALID)
		{
		}

		float* gentexcoords(const void* coords, int vertex_count) const
		{
			GLfloat* tcoord = NULL;
			if (m_mode == BITMAP_WRAP || m_mode == BITMAP_CLAMP)
			{
				tcoord = new GLfloat[2 * vertex_count];
				float* vcoord = (float*) coords;
				for (int i = 0, n = 2 * vertex_count; i < n; i = i + 2)
				{
					tcoord[i] = vcoord[i] * m_pS[0] + vcoord[i + 1] * m_pS[1] + m_pS[3];
					tcoord[i + 1] = vcoord[i] * m_pT[0] + vcoord[i + 1] * m_pT[1] + m_pT[3];
				}
			}
			return tcoord;
		}
		
		void	apply(const shader& sh) const
		// Push our style into OpenGL.
		{
			assert(m_mode != INVALID);
			
			if (m_mode == COLOR)
			{
				glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
				sh.set_uniform4v("uColor", 1, m_color);
				glDisable(GL_TEXTURE_2D);
			}
			else
			if (m_mode == BITMAP_WRAP || m_mode == BITMAP_CLAMP)
			{
				assert(m_bitmap_info != NULL);
				if (m_bitmap_info == NULL)
				{
					glDisable(GL_TEXTURE_2D);
				}
				else
				{
					glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
					sh.set_uniform4v("uColor", 1, m_color);

					m_bitmap_info->upload();
					if (m_mode == BITMAP_CLAMP)
					{	
						glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
						glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
					}
					else
					{
						assert(m_mode == BITMAP_WRAP);
						glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
						glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
					}

					// Set up the bitmap matrix for texgen.
					float p2w = (float) p2(m_bitmap_info->get_width());	// needs "+ 1" ?
					float p2h = (float) p2(m_bitmap_info->get_height());	// needs "+ 1" ?

					const bakeinflash::matrix&	m = m_bitmap_matrix;
					m_pS[0] = m.m_[0][0] / p2w;
					m_pS[1] = m.m_[0][1] / p2w;
					m_pS[2] = 0;
					m_pS[3] = m.m_[0][2] / p2w;
					m_pT[0] = m.m_[1][0] / p2h;
					m_pT[1] = m.m_[1][1] / p2h;
					m_pT[2] = 0;
					m_pT[3] = m.m_[1][2] / p2h;
				}
			}
		}

		void	disable() { m_mode = INVALID; }

		void	set_color(bakeinflash::rgba color)
		{
			m_mode = COLOR; 
			m_color[0] = color.m_r / 255.0f; 
			m_color[1] = color.m_g / 255.0f; 
			m_color[2] = color.m_b / 255.0f; 
			m_color[3] = color.m_a / 255.0f; 
		}

		void	set_bitmap(bakeinflash::bitmap_info* bi, const bakeinflash::matrix& m, bitmap_wrap_mode wm, const bakeinflash::cxform& color_transform)
		{
			m_mode = (wm == WRAP_REPEAT) ? BITMAP_WRAP : BITMAP_CLAMP;
			m_bitmap_info = bi;
			m_bitmap_matrix = m;

			m_color[0] = color_transform.m_[0][0];
			m_color[1] = color_transform.m_[1][0];
			m_color[2] = color_transform.m_[2][0];
			m_color[3] = color_transform.m_[3][0];

			//
			// корректировать цета
			//
			m_color[0] += color_transform.m_[0][1] / 255.0f;
			m_color[1] += color_transform.m_[1][1] / 255.0f;
			m_color[2] += color_transform.m_[2][1] / 255.0f;
			m_color[3] += color_transform.m_[3][1] / 255.0f;

			// так как используется GL_ONE надо вычислить компоненты цвета заранее применив ALFA
			m_color[0] *= m_color[3]; 
			m_color[1] *= m_color[3]; 
			m_color[2] *= m_color[3]; 
		}
	
		bool	is_valid() const { return m_mode != INVALID; }
	};


	// Style state.
	enum style_index
	{
		LEFT_STYLE = 0,
		RIGHT_STYLE,
		LINE_STYLE,

		STYLE_COUNT
	};
	fill_style	m_current_styles[STYLE_COUNT];


	bakeinflash::bitmap_info*	create_bitmap_info(image::image_base* im)
	// Given an image, returns a pointer to a bitmap_info struct
	// that can later be passed to fill_styleX_bitmap(), to set a
	// bitmap fill style.
	{
		if (im)
		{
			return new bitmap_info_ogl(im);
		}
		return new bitmap_info_ogl();
	}

	bakeinflash::video_handler*	create_video_handler()
	{
		return new video_handler_ogles();
	}

	void clear()
	{
		glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
		glClear(GL_COLOR_BUFFER_BIT);
	}

	void read_buffer(int x, int y, int w, int h, Uint8* buf)
	{
		glReadPixels(x, y, w, h, GL_RGBA, GL_UNSIGNED_BYTE, buf);	// window coords
	}

	void	begin_display(
		bakeinflash::rgba background_color,
		int viewport_x0, int viewport_y0,
		int viewport_width, int viewport_height,
		float x0, float x1, float y0, float y1)
	// Set up to render a full frame from a movie and fills the
	// background.	Sets up necessary transforms, to scale the
	// movie to fit within the given dimensions.  Call
	// end_display() when you're done.
	//
	// The rectangle (viewport_x0, viewport_y0, viewport_x0 +
	// viewport_width, viewport_y0 + viewport_height) defines the
	// window coordinates taken up by the movie.
	//
	// The rectangle (x0, y0, x1, y1) defines the pixel
	// coordinates of the movie that correspond to the viewport
	// bounds.
	{
		m_display_width = fabsf(x1 - x0);
		m_display_height = fabsf(y1 - y0);

		glClearColor(background_color.m_r / 255.0f, background_color.m_g / 255.0f, background_color.m_b / 255.0f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);
		glViewport(viewport_x0, viewport_y0, viewport_width, viewport_height);

		float nnear = -100;		// 100 nested masks ?

		glEnable(GL_BLEND);
	}

	void	end_display()
	// Clean up after rendering a frame.  Client program is still
	// responsible for calling glSwapBuffers() or whatever.
	{
	}


	void	set_matrix(const bakeinflash::matrix& m)
	// Set the current transform for mesh & line-strip rendering.
	{
		m_current_matrix = m;
	}


	void	set_cxform(const bakeinflash::cxform& cx)
	// Set the current color transform for mesh & line-strip rendering.
	{
		m_current_cxform = cx;
	}

	void	set_rgba(bakeinflash::rgba* color)
	{
		m_current_rgba = color;
	}

	void	fill_style_disable(int fill_side)
	// Don't fill on the {0 == left, 1 == right} side of a path.
	{
		assert(fill_side >= 0 && fill_side < 2);
		m_current_styles[fill_side].disable();
	}


	void	line_style_disable()
	// Don't draw a line on this path.
	{
		m_current_styles[LINE_STYLE].disable();
	}


	void	fill_style_color(int fill_side, const bakeinflash::rgba& color)
	// Set fill style for the left interior of the shape.  If
	// enable is false, turn off fill for the left interior.
	{
		assert(fill_side >= 0 && fill_side < 2);

		m_current_styles[fill_side].set_color(m_current_cxform.transform(color));
	}


	void	line_style_color(bakeinflash::rgba color)
	// Set the line style of the shape.  If enable is false, turn
	// off lines for following curve segments.
	{
		m_current_styles[LINE_STYLE].set_color(m_current_cxform.transform(color));
	}


	void	fill_style_bitmap(int fill_side, bakeinflash::bitmap_info* bi, const bakeinflash::matrix& m,
		bitmap_wrap_mode wm, bitmap_blend_mode bm)
	{
		assert(fill_side >= 0 && fill_side < 2);
		m_current_styles[fill_side].set_bitmap(bi, m, wm, m_current_cxform);
	}
	
	void	line_style_width(float width)
	{
		m_current_styles[LINE_STYLE].m_width = width;
	}

	void	line_style_caps(int caps_style)
	{
		m_current_styles[LINE_STYLE].m_caps_style = caps_style;
	}


	void	draw_mesh_primitive(int primitive_type, const void* coords, int vertex_count)
	// Helper for draw_mesh_strip and draw_triangle_list.
	{
		// Set up current style.
//		m_current_styles[LEFT_STYLE].apply();

		// implements set_RGB
		if (m_current_rgba)
		{
//vv			glColor4f(m_current_rgba->m_r / 255.0f, m_current_rgba->m_g / 255.0f, m_current_rgba->m_b / 255.0f, m_current_rgba->m_a / 255.0f);
	//		glDisable(GL_TEXTURE_2D);
		}

		const fill_style& fs = m_current_styles[LEFT_STYLE];
		const shader& sh = fs.m_mode == fill_style::COLOR ? m_shader_shape : m_shader_bitmap;
		sh.attach();

		GLint position_attrib_index = sh.getAttribLocation("vCoord");
		glEnableVertexAttribArray(position_attrib_index);
		glVertexAttribPointer(position_attrib_index, 2, GL_FLOAT, false, 0, coords);

		fs.apply(sh);
		GLint texcoord_attrib_index = -1;
		float* tcoord = m_current_styles[LEFT_STYLE].gentexcoords(coords, vertex_count);
		if (tcoord)
		{
			texcoord_attrib_index = sh.getAttribLocation("tCoord");
			glEnableVertexAttribArray(texcoord_attrib_index); // Attribute indexes were received from calls to glGetAttribLocation, or passed into glBindAttribLocation.
			glVertexAttribPointer(texcoord_attrib_index, 2, GL_FLOAT, false, 0, tcoord); // texcoords_data is a float*, 2 per vertex, representing UV coordinates.
		}

		bakeinflash::matrix m = m_current_matrix;
		m.concatenate_xyscale(2 / m_display_width, 2 / m_display_height);
		m.m_[0][2] = m.m_[0][2] / m_display_width * 2 - 1;
		m.m_[1][2] = m.m_[1][2] / m_display_height * 2 - 1;

		sh.set_uniform3v("xMat", 2, &m.m_[0][0]);
		glDrawArrays(primitive_type, 0, vertex_count);
		sh.detach();
        
		//
		//	add color, sample
		/*
		const bakeinflash::rgba& ac = m_current_styles[LEFT_STYLE].m_additive_color;
		if (ac.m_a + ac.m_r + ac.m_g + ac.m_b > 0)
		{
			glBlendFunc(GL_ZERO, GL_ONE_MINUS_SRC_ALPHA);
			glDisable(GL_TEXTURE_2D);
			glColor4f(ac.m_r / 255.0f, ac.m_g / 255.0f, ac.m_b / 255.0f, ac.m_a / 255.0f);
			glDrawArrays(primitive_type, 0, vertex_count);
		}
		*/

		if (tcoord)
		{
			glDisableVertexAttribArray(texcoord_attrib_index);
			delete [] tcoord;
		}
		glDisableVertexAttribArray(position_attrib_index);
	}

	void draw_mesh_strip(const void* coords, int vertex_count)
	{
		draw_mesh_primitive(GL_TRIANGLE_STRIP, coords, vertex_count);
	}
			
	void	draw_triangle_list(const void* coords, int vertex_count)
	{
		draw_mesh_primitive(GL_TRIANGLES, coords, vertex_count);
	}

	void	draw_line_strip(const void* coords, int vertex_count)
	// Draw the line strip formed by the sequence of points.
	{
		int caps = m_current_styles[LINE_STYLE].m_caps_style;
		float* vertexbuf = NULL;

		if (m_hasStencilBuffer == false && m_mask_level > 0)
		{
			float* co = (float*) coords;
			vertexbuf = new float[sizeof(float) * 3 * vertex_count];
			for (int i = 0; i < vertex_count; i++)
			{
				vertexbuf[i * 3 + 0] = co[i * 2 + 0];
				vertexbuf[i * 3 + 1] = co[i * 2 + 1];
				vertexbuf[i * 3 + 2] = (float) m_mask_level;
			}
//vv			glVertexPointer(3, GL_FLOAT, sizeof(float) * 3, vertexbuf);
		}
		else
		{
//vv			glVertexPointer(2, GL_FLOAT, sizeof(float) * 2, coords);
		}

		m_shader_line.attach();

		GLint position_attrib_index = m_shader_line.getAttribLocation("vCoord");
		glEnableVertexAttribArray(position_attrib_index);
		glVertexAttribPointer(position_attrib_index, 2, GL_FLOAT, false, 0, coords);

		bakeinflash::matrix m = m_current_matrix;
		m.concatenate_xyscale(2 / m_display_width, 2 / m_display_height);
		m.m_[0][2] = m.m_[0][2] / m_display_width * 2 - 1;
		m.m_[1][2] = m.m_[1][2] / m_display_height * 2 - 1;

		m_shader_line.set_uniform3v("xMat", 2, &m.m_[0][0]);

		const fill_style& fs = m_current_styles[LINE_STYLE];
		m_shader_line.set_uniform4v("uColor", 1, fs.m_color);

		glDrawArrays(GL_LINE_STRIP, 0, vertex_count);
		m_shader_line.detach();

		glDisableVertexAttribArray(position_attrib_index);

		delete[] vertexbuf;
	}


	void	draw_bitmap(
		const bakeinflash::matrix& m,
		bakeinflash::bitmap_info* bi,
		const bakeinflash::rect& coords,
		const bakeinflash::rect& uv_coords,
		bakeinflash::rgba color)
	// Draw a rectangle textured with the given bitmap, with the
	// given color.	 Apply given transform; ignore any currently
	// set transforms.
	//
	// Intended for textured glyph rendering.
	{
		assert(bi);
		bi->upload();

		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

		GLfloat squareVertices[12];
		GLfloat squareTextureCoords[8];
		if (m_hasStencilBuffer == false && m_mask_level > 0)
		{
			squareVertices[0] = coords.m_x_min;
			squareVertices[1] = coords.m_y_min;
			squareVertices[2] = (float) m_mask_level;
			squareVertices[3] = coords.m_x_max;
			squareVertices[4] = coords.m_y_min;
			squareVertices[5] = (float) m_mask_level;
			squareVertices[6] = coords.m_x_min;
			squareVertices[7] = coords.m_y_max;
			squareVertices[8] = (float) m_mask_level;
			squareVertices[9] = coords.m_x_max;
			squareVertices[10] = coords.m_y_max;
			squareVertices[11] = (float) m_mask_level;
		
			squareTextureCoords[0] = uv_coords.m_x_min;
			squareTextureCoords[1] = uv_coords.m_y_min;
			squareTextureCoords[2] = uv_coords.m_x_max;
			squareTextureCoords[3] = uv_coords.m_y_min;
			squareTextureCoords[4] = uv_coords.m_x_min;
			squareTextureCoords[5] = uv_coords.m_y_max;
			squareTextureCoords[6] = uv_coords.m_x_max;
			squareTextureCoords[7] = uv_coords.m_y_max;
		}
		else
		{
			squareVertices[0] = coords.m_x_min;
			squareVertices[1] = coords.m_y_min;
			squareVertices[2] = coords.m_x_max;
			squareVertices[3] = coords.m_y_min;
			squareVertices[4] = coords.m_x_min;
			squareVertices[5] = coords.m_y_max;
			squareVertices[6] = coords.m_x_max;
			squareVertices[7] = coords.m_y_max;
		
			squareTextureCoords[0] = uv_coords.m_x_min;
			squareTextureCoords[1] = uv_coords.m_y_min;
			squareTextureCoords[2] = uv_coords.m_x_max;
			squareTextureCoords[3] = uv_coords.m_y_min;
			squareTextureCoords[4] = uv_coords.m_x_min;
			squareTextureCoords[5] = uv_coords.m_y_max;
			squareTextureCoords[6] = uv_coords.m_x_max;
			squareTextureCoords[7] = uv_coords.m_y_max;
		}


		const shader& sh = bi->get_bpp() == 1 ? m_shader_text : m_shader_bitmap;
		sh.attach();

		GLint position_attrib_index = sh.getAttribLocation("vCoord");
		glEnableVertexAttribArray(position_attrib_index);
		glVertexAttribPointer(position_attrib_index, 2, GL_FLOAT, false, 0, squareVertices);

		GLint texcoord_attrib_index = sh.getAttribLocation("tCoord");
		glEnableVertexAttribArray(texcoord_attrib_index); // Attribute indexes were received from calls to glGetAttribLocation, or passed into glBindAttribLocation.
		glVertexAttribPointer(texcoord_attrib_index, 2, GL_FLOAT, false, 0, squareTextureCoords); // texcoords_data is a float*, 2 per vertex, representing UV coordinates.

		bakeinflash::matrix mm = m;
		mm.concatenate_xyscale(2 / m_display_width, 2 / m_display_height);
		mm.m_[0][2] = mm.m_[0][2] / m_display_width * 2 - 1;
		mm.m_[1][2] = mm.m_[1][2] / m_display_height * 2 - 1;

		sh.set_uniform3v("xMat", 2, &mm.m_[0][0]);

		float cc[4];	cc[0] = color.m_r / 255.0f;	cc[1] = color.m_g / 255.0f;	cc[2] = color.m_b / 255.0f;	cc[3] = color.m_a / 255.0f;
		sh.set_uniform4v("uColor", 1, cc);

		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
		sh.detach();
        
		//
		//	add color, sample
		/*
		const bakeinflash::rgba& ac = m_current_styles[LEFT_STYLE].m_additive_color;
		if (ac.m_a + ac.m_r + ac.m_g + ac.m_b > 0)
		{
			glBlendFunc(GL_ZERO, GL_ONE_MINUS_SRC_ALPHA);
			glDisable(GL_TEXTURE_2D);
			glColor4f(ac.m_r / 255.0f, ac.m_g / 255.0f, ac.m_b / 255.0f, ac.m_a / 255.0f);
			glDrawArrays(primitive_type, 0, vertex_count);
		}
		*/

		glDisableVertexAttribArray(texcoord_attrib_index);
		glDisableVertexAttribArray(position_attrib_index);
		glDisable(GL_TEXTURE_2D);
	}
	
	bool test_stencil_buffer(const bakeinflash::rect& bound, Uint8 pattern)
	{
		return false;
/*		
		// get viewport size
		GLint vp[4]; 
		glGetIntegerv(GL_VIEWPORT, vp); 
		int vp_width = vp[2];
		int vp_height = vp[3];

		bool ret = false;

		int x0 = (int) bound.m_x_min;
		int y0 = (int) bound.m_y_min;
		int width = (int) bound.m_x_max - x0;
		int height = (int) bound.m_y_max - y0;

		if (width > 0 && height > 0 &&
			x0 >= 0 && x0 + width <= vp_width &&
			y0 >= 0 && y0 + height <= vp_height)
		{
			int bufsize = width * height;
			Uint8* buf = (Uint8*) malloc(4 * bufsize);

			assert(0);	//TODO
//			glReadPixels(x0, vp[3] - y0 - height, width, height, GL_STENCIL_INDEX, GL_UNSIGNED_BYTE, buf);

			for (int i = 0; i < bufsize; i++)
			{
				if (buf[i] == pattern)
				{
					ret = true;
					break;
				}
			}

			free(buf);
		}

		return ret;*/
	}

	void begin_submit_mask()
	{
		if (m_hasStencilBuffer)
		{
			if (m_mask_level == 0)
			{
				glEnable(GL_STENCIL_TEST);
				glClearStencil(0);
				glClear(GL_STENCIL_BUFFER_BIT);
			}
			
			// disable framebuffer writes
			glColorMask(0, 0, 0, 0);
			
			// we set the stencil buffer to 'm_mask_level+1' 
			// where we draw any polygon and stencil buffer is 'm_mask_level'
			glStencilFunc(GL_EQUAL, m_mask_level++, 0xFF);
			glStencilOp(GL_KEEP, GL_KEEP, GL_INCR); 
		}
		else
		{
			glEnable(GL_DEPTH_TEST); // to enable writing to the depth buffer
			glDepthMask(GL_TRUE);    // to allow writes to the depth buffer
			if (m_mask_level == 0)
			{
				//glClearDepth(1);
				glClear(GL_DEPTH_BUFFER_BIT); // for a fresh start
				glDepthFunc(GL_ALWAYS);  // to ensure everything you draw passes	
			}
			else
			{
				glDepthFunc(GL_GEQUAL);
			}
			glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);   // so that whatever we draw isn't actually visible
			m_mask_level++;
		}		
	}
	
	// called after begin_submit_mask and the drawing of mask polygons
	void end_submit_mask()
	{	     
		if (m_hasStencilBuffer)
		{
			// enable framebuffer writes
			glColorMask(1, 1, 1, 1);
			
			// we draw only where the stencil is m_mask_level (where the current mask was drawn)
			glStencilFunc(GL_EQUAL, m_mask_level, 0xFF);
			glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);	
		}
		else
		{
			glDepthMask(GL_FALSE);    // to allow writes to the depth buffer
			glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
			glDepthFunc(GL_EQUAL);
		}
	}
	

	void disable_mask()
	{	     
		assert(m_mask_level > 0);
		
		if (--m_mask_level == 0)
		{
			glDisable(m_hasStencilBuffer ? GL_STENCIL_TEST : GL_DEPTH_TEST); 
			return;
		}

		// nested mask
		if (m_hasStencilBuffer)
		{
			// begin submit previous mask
			
			glColorMask(0, 0, 0, 0);
			
			// we set the stencil buffer to 'm_mask_level' 
			// where the stencil buffer m_mask_level + 1
			glStencilFunc(GL_EQUAL, m_mask_level + 1, 0xFF);
			glStencilOp(GL_KEEP, GL_KEEP, GL_DECR); 
			
			// draw the quad to fill stencil buffer
			Sint16 squareVertices[8]; 
			squareVertices[0] = 0;
			squareVertices[1] = 0;
			squareVertices[2] = (Sint16) m_display_width;
			squareVertices[3] = 0;
			squareVertices[4] = 0;
			squareVertices[5] = (Sint16) m_display_height;
			squareVertices[6] = (Sint16) m_display_width;
			squareVertices[7] = (Sint16) m_display_height;
			
//vv			glVertexPointer(2, GL_SHORT, 0, squareVertices);
//vv			glEnableClientState(GL_VERTEX_ARRAY);
			glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
			
//vv			glDisableClientState(GL_VERTEX_ARRAY);
		}		
		end_submit_mask();
	}

	

	bool is_visible(const bakeinflash::rect& bound)
	{
		bakeinflash::rect viewport;
		viewport.m_x_min = 0;
		viewport.m_y_min = 0;
		viewport.m_x_max = m_display_width;
		viewport.m_y_max = m_display_height;
		return viewport.bound_test(bound);
	}

#ifdef SDL_CURSOR_HANDLING
	// SDL cursor handling.
	sdl_cursor_handler m_cursor_handler;
	
	void set_cursor(cursor_type cursor)
	{
		m_cursor_handler.set_cursor(cursor);
	}
#endif  // SDL_CURSOR_HANDLING

};	// end struct render_handler_ogles


// bitmap_info_ogl implementation

bitmap_info_ogl::bitmap_info_ogl() :
	m_texture_id(0),
	m_width(0),
	m_height(0),
	m_suspended_image(0),
	m_keep_alive_source_image(false),
	m_xratio(0.0f),
	m_yratio(0.0f),
	m_bpp(0)
{
}

bitmap_info_ogl::bitmap_info_ogl(image::image_base* im) :
	m_texture_id(0),
	m_width(im->m_width),
	m_height(im->m_height),
	m_suspended_image(im),
	m_keep_alive_source_image(false),
	m_xratio(0.0f),
	m_yratio(0.0f),
	m_bpp(0)
{
	if (im)
	{
		switch (im->m_type)
		{
			case image::image_base::RGB: 	
				m_bpp = 3;
				break;
			case image::image_base::RGBA: 
				m_bpp = 4;
				break;
			case image::image_base::ALPHA: 
				m_bpp = 1;
				break;
		}
	}

#ifdef DEBUG_BITMAPS
		myprintf("new bitmap_info_ogl %dx%d\n", m_width, m_height);
#endif
		if (im)
		{
			m_xratio = im->m_width / (float) p2(im->m_width);
			m_yratio = im->m_height / (float) p2(im->m_height);
		}
}

void bitmap_info_ogl::update(const Uint8* buf, int width, int height)
{
	if (m_texture_id > 0)
	{
		glBindTexture(GL_TEXTURE_2D, m_texture_id);
		glEnable(GL_TEXTURE_2D);
		glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, buf);
		glDisable(GL_TEXTURE_2D);
	}
}

// upload image to opengl texture memory
void bitmap_info_ogl::upload()
{
	if (m_texture_id == 0)
	{
		//assert(m_suspended_image);
		if (m_suspended_image == NULL)
		{
			return;
		}

		// Create the texture.
		glEnable(GL_TEXTURE_2D);
		glGenTextures(1, (GLuint*) &m_texture_id);
		glBindTexture(GL_TEXTURE_2D, m_texture_id);

#ifdef DEBUG_BITMAPS
		myprintf("layout tex=%d, %dx%d\n", m_texture_id, m_suspended_image->m_width, m_suspended_image->m_height);
#endif

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

		m_width = m_suspended_image->m_width;
		m_height = m_suspended_image->m_height;
		int	p2w = p2(m_width);
		int	p2h = p2(m_height);

		// in some openGL implementation the following
		// glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, p2w, p2h, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
		// does not clear texture memory !!!
		switch (m_suspended_image->m_type)
		{
			case image::image_base::RGB:
			{
				// rgb ==> rgba
				int bufsize = 4 * p2w * p2h;
				Uint8* buf = (Uint8*) malloc(bufsize);
				memset(buf, 0, bufsize);

				for (int y = 0; y < m_height; y++)
				{
					// sometimes pitch < 3*width ??
					const Uint8* src = m_suspended_image->m_data + y * m_suspended_image->m_pitch;
					Uint8* dst = buf + 4 * p2w * y;
					for (int x = 0; x < m_width; x++)
					{
						*dst++ = *src++;
						*dst++ = *src++;
						*dst++ = *src++;
						*dst++ = 255;
					}
				}

				glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, p2w, p2h, 0, GL_RGBA, GL_UNSIGNED_BYTE, buf);
				free(buf);
				break;
			}

			case image::image_base::RGBA:
			{
				if (p2w != m_width || p2h != m_height)
				{
					// rgba ==> p2w-p2h rgba
					int bufsize = 4 * p2w * p2h;
					Uint8* buf = (Uint8*) malloc(bufsize);
					memset(buf, 0, bufsize);

					const Uint8* src = m_suspended_image->m_data;
					Uint8* dst = buf;
					for (int y = 0; y < m_height; y++)
					{
						// sometimes pitch < 3*width ??
						memcpy(dst, src, m_suspended_image->m_pitch);
						src += m_suspended_image->m_pitch;
						dst += 4 * p2w;
					}
					glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, p2w, p2h, 0, GL_RGBA, GL_UNSIGNED_BYTE, buf);
					free(buf);
				}
				else
				{
					glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, p2w, p2h, 0, GL_RGBA, GL_UNSIGNED_BYTE, m_suspended_image->m_data);
				}
				break;
			}

			case image::image_base::ALPHA:
			{
				if (p2w != m_width || p2h != m_height)
				{
					// alpha ==> p2w-p2h alpha
					int bufsize = p2w * p2h;
					Uint8* buf = (Uint8*) malloc(bufsize);
					memset(buf, 0, bufsize);

					const Uint8* src = m_suspended_image->m_data;
					Uint8* dst = buf;
					for (int y = 0; y < m_height; y++)
					{
						// sometimes pitch < 3*width ??
						memcpy(dst, src, m_suspended_image->m_pitch);
						src += m_suspended_image->m_pitch;
						dst += p2w;
					}
					glTexImage2D(GL_TEXTURE_2D, 0, GL_ALPHA, p2w, p2h, 0, GL_ALPHA, GL_UNSIGNED_BYTE, buf);
					free(buf);
				}
				else
				{
					glTexImage2D(GL_TEXTURE_2D, 0, GL_ALPHA, p2w, p2h, 0, GL_ALPHA, GL_UNSIGNED_BYTE, m_suspended_image->m_data);
				}
				break;
			}

			default:
				assert(0);
		}

		if (m_keep_alive_source_image == false)
		{
			delete m_suspended_image;
		}
		m_suspended_image = NULL;
	}
	else
	{
		glBindTexture(GL_TEXTURE_2D, m_texture_id);
		glEnable(GL_TEXTURE_2D);
	}
}

}	// render_handler_ogles


//
//
//

shader::shader() :
	m_prog(0)
{
}

shader::~shader()
{
	if (m_prog > 0)
	{
		glUseProgram(0);
		glDeleteProgram(m_prog);
	}
}

void shader::attach() const
{
	assert(m_prog > 0);
	glUseProgram(m_prog);
}

void shader::detach() const
{
	glUseProgram(0);
}

GLint shader::get_uniform(const char* name) const
{
	GLint u = glGetUniformLocation(m_prog, name);
	if (u < 0)
	{
		bakeinflash::myprintf("no uniform variable: %s\n", name);
	}
	return u;
}

void shader::set_uniform(const char* name, GLfloat val) const
{
	glUniform1f(get_uniform(name), val);
}

void shader::set_uniform(const char* name, GLfloat val1, GLfloat val2) const
{
	glUniform2f(get_uniform(name), val1, val2);
}

void shader::set_uniform(const char* name, GLfloat val1, GLfloat val2, GLfloat val3) const
{
	glUniform3f(get_uniform(name), val1, val2, val3);
}

void shader::set_uniform(const char* name, GLfloat val1, GLfloat val2, GLfloat val3, GLfloat val4) const
{
	glUniform4f(get_uniform(name), val1, val2, val3, val3);
}

void shader::set_uniform4mat(const char* name, GLfloat mat[16]) const
{
	glUniformMatrix4fv(get_uniform(name), 1, false, mat);
}

void shader::set_uniform2mat(const char* name, GLfloat mat[4]) const
{
	glUniformMatrix2fv(get_uniform(name), 1, false, mat);
}


void shader::set_tex(const char* name, int val) const
{
	GLint u = glGetUniformLocation(m_prog, name);
	if (u < 0)
	{
		bakeinflash::myprintf("no uniform variable: %s\n", name);
		return;
	}
	glUniform1i(u, val);
}

void shader::set_uniform1v(const char* name, int size, GLfloat* val) const
{
	GLint u = glGetUniformLocation(m_prog, name);
	if (u < 0)
	{
		bakeinflash::myprintf("no uniform variable: %s\n", name);
		return;
	}
	glUniform1fv(u, size, val);
}

void shader::set_uniform3v(const char* name, int size, const GLfloat* val) const
{
	GLint u = glGetUniformLocation(m_prog, name);
	if (u < 0)
	{
		bakeinflash::myprintf("no uniform variable: %s\n", name);
		return;
	}
	//	printf("%f %f %f\n", val[0], val[1], val[2]);
	glUniform3fv(u, size, val);
}

void shader::set_uniform4v(const char* name, int size, GLfloat* val) const
{
	GLint u = glGetUniformLocation(m_prog, name);
	if (u < 0)
	{
		bakeinflash::myprintf("no uniform variable: %s\n", name);
		return;
	}
	glUniform4fv(u, size, val);
}

int shader::compile(const char* vs_source, const char* fs_source)
{
	if (m_prog == 0)
	{
		m_prog = glCreateProgram();
		assert(m_prog > 0);

		if (vs_source)
		{
			GLuint vs = glCreateShader(GL_VERTEX_SHADER);

			glShaderSource(vs, 1, (const GLchar**)&vs_source, NULL);

			glCompileShader(vs);
			checkGlShader(vs);

			glAttachShader(m_prog, vs);
		}

		if (fs_source)
		{
			GLuint fs = glCreateShader(GL_FRAGMENT_SHADER);

			glShaderSource(fs, 1, (const GLchar**)&fs_source, NULL);

			glCompileShader(fs);
			checkGlShader(fs);

			glAttachShader(m_prog, fs);
		}

		glLinkProgram(m_prog);
		checkGlProgram(m_prog);
	}
	return m_prog;
}

int shader::getAttribLocation(const char* name) const
{
	assert(m_prog > 0);
	int id = glGetAttribLocation(m_prog, name);
	if (id == -1)
	{
		bakeinflash::myprintf("glGetAttribLocation: no '%s'\n", name);
	}
	return id;
}


namespace bakeinflash
{
	render_handler*	create_render_handler_ogles(bool hasStencilBuffer)
	// Factory.
	{
		return new render_handler_ogles::render_handler_ogles(hasStencilBuffer);
	}
};

#endif	// USE_SDL

// Local Variables:
// mode: C++
// c-basic-offset: 8 
// tab-width: 8
// indent-tabs-mode: t
// End:
