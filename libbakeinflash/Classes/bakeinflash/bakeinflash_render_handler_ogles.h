#ifndef BAKEINFLASH_OPENGL_ES_H
#define BAKEINFLASH_OPENGL_ES_H

#if ANDROID == 1
#define GL_BGRA                           0x80E1
//#include <GLES/gl.h>		// for ES1
#include <GLES2/gl2.h>		// for ES2
#elif TU_USE_SDL
//#define GL_GLEXT_PROTOTYPES 1
#include <SDL2/SDL.h>  // for cursor handling & the scanning for extensions.
//#include <SDL2/SDL_opengles2.h>	// for opengl const
#include <SDL2/SDL_opengl.h>	// for opengl const
#else
// #include <OpenGLES/ES1/glext.h>
#include <OpenGLES/ES2/gl.h>
#include <OpenGLES/ES2/glext.h>
#endif

void checkGlError();
void checkGlProgram(GLuint prog);
void checkGlShader(GLuint prog);


struct shader
{
	int m_prog;

	shader();
	~shader();

	int compile(const char* vs_source, const char* fs_source);
	void attach() const;
	void detach() const;

	int get_uniform(const char* name) const;
	void set_uniform(const char* name, GLfloat val) const;
	void set_uniform(const char* name, GLfloat val1, GLfloat val2) const;
	void set_uniform(const char* name, GLfloat val1, GLfloat val2, GLfloat val3) const;
	void set_uniform(const char* name, GLfloat val1, GLfloat val2, GLfloat val3, GLfloat val4) const;
	void set_tex(const char* name, int texture_unit) const;
	void set_uniform1v(const char* name, int size, GLfloat* val) const;
	void set_uniform3v(const char* name, int size, const GLfloat* val) const;
	void set_uniform4v(const char* name, int size, GLfloat* val) const;
	void set_uniform4mat(const char* name, GLfloat mat[16]) const;
	void set_uniform2mat(const char* name, GLfloat mat[4]) const;
	int getAttribLocation(const char* name) const;
	bool is_ready() const { return m_prog > 0; }

};


#endif

