#pragma once
#ifdef USE_GLES2
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>

// Desktop entry points used by the shared renderer, implemented using ES 2.0.
#define GL_RGBA8 GL_RGBA
#define GL_R8 GL_LUMINANCE
#define GL_RED GL_LUMINANCE
#define GL_DEPTH_COMPONENT24 0x81A6
#define GL_READ_FRAMEBUFFER 0x8CA8
#define GL_DRAW_FRAMEBUFFER 0x8CA9
#define GL_READ_FRAMEBUFFER_BINDING 0x8CAA
#define GL_DRAW_FRAMEBUFFER_BINDING GL_FRAMEBUFFER_BINDING
#define GL_VERTEX_ARRAY_BINDING 0x85B5
#define GL_MAX_SAMPLES 0x8D57
#define GL_TEXTURE_BASE_LEVEL 0x813C
#define GL_TEXTURE_MAX_LEVEL 0x813D
#define GL_TEXTURE_MIN_LOD 0x813A
#define GL_TEXTURE_MAX_LOD 0x813B
#define GL_TEXTURE_LOD_BIAS 0x8501
#define GL_TEXTURE_COMPARE_MODE 0x884C
#define GL_DEPTH_CLAMP 0x864F
#define GL_COLOR_LOGIC_OP 0x0BF2
#define GL_CLEAR 0x1500
#define GL_AND 0x1501
#define GL_AND_REVERSE 0x1502
#define GL_COPY 0x1503
#define GL_AND_INVERTED 0x1504
#define GL_NOOP 0x1505
#define GL_XOR 0x1506
#define GL_OR 0x1507
#define GL_NOR 0x1508
#define GL_EQUIV 0x1509
#define GL_INVERT 0x150A
#define GL_OR_REVERSE 0x150B
#define GL_COPY_INVERTED 0x150C
#define GL_OR_INVERTED 0x150D
#define GL_NAND 0x150E
#define GL_SET 0x150F

void esGenVertexArrays(GLsizei, GLuint*);
void esBindVertexArray(GLuint);
void esVertexAttribPointer(GLuint, GLint, GLenum, GLboolean, GLsizei, const void*);
void esEnableVertexAttribArray(GLuint);
void esBindFramebuffer(GLenum, GLuint);
void esFramebufferTexture2D(GLenum, GLenum, GLenum, GLuint, GLint);
void esGetIntegerv(GLenum, GLint*);
void esReadPixels(GLint, GLint, GLsizei, GLsizei, GLenum, GLenum, void*);
void esGenSamplers(GLsizei, GLuint*);
void esSamplerParameteri(GLuint, GLenum, GLint);
void esSamplerParameterf(GLuint, GLenum, GLfloat);
void esBindSampler(GLuint, GLuint);
void esTexParameteri(GLenum, GLenum, GLint);
void esTexImage2D(GLenum, GLint, GLint, GLsizei, GLsizei, GLint, GLenum, GLenum, const void*);
void esEnable(GLenum);
void esDisable(GLenum);
void esLogicOp(GLenum);
void esBlitFramebuffer(GLint, GLint, GLint, GLint, GLint, GLint, GLint, GLint, GLbitfield, GLenum);
void esRenderbufferStorageMultisample(GLenum, GLsizei, GLenum, GLsizei, GLsizei);
void esDrawQuad();
void esShaderSource(GLuint, GLsizei, const GLchar* const*, const GLint*);
void esLinkProgram(GLuint);

#ifndef GLES_IMPLEMENTATION
#define glGenVertexArrays esGenVertexArrays
#define glBindVertexArray esBindVertexArray
#define glVertexAttribPointer esVertexAttribPointer
#define glEnableVertexAttribArray esEnableVertexAttribArray
#define glBindFramebuffer esBindFramebuffer
#define glFramebufferTexture2D esFramebufferTexture2D
#define glGetIntegerv esGetIntegerv
#define glReadPixels esReadPixels
#define glGenSamplers esGenSamplers
#define glSamplerParameteri esSamplerParameteri
#define glSamplerParameterf esSamplerParameterf
#define glBindSampler esBindSampler
#define glTexParameteri esTexParameteri
#define glTexImage2D esTexImage2D
#define glEnable esEnable
#define glDisable esDisable
#define glLogicOp esLogicOp
#define glBlitFramebuffer esBlitFramebuffer
#define glRenderbufferStorageMultisample esRenderbufferStorageMultisample
#define glClearDepth glClearDepthf
#define glShaderSource esShaderSource
#define glLinkProgram esLinkProgram
#endif
#elif defined(_WIN32)
#include "glad.h"
#else
#define GL_GLEXT_PROTOTYPES
#include <GL/gl.h>
#include <GL/glext.h>
#endif
