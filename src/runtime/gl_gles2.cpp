#ifdef USE_GLES2
#define GLES_IMPLEMENTATION
#include "gl_api.h"
#include <algorithm>
#include <array>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <unordered_map>
#include <vector>

namespace {
struct Attr { bool enabled = false; GLint size = 4; GLenum type = GL_FLOAT; GLboolean norm = GL_FALSE; GLsizei stride = 0; const void* ptr = nullptr; GLuint buffer = 0; };
struct Vao { std::array<Attr,8> attrs; GLuint indices = 0; };
std::vector<Vao> vaos(1);
GLuint vao = 0, read_fbo = 0, draw_fbo = 0;
struct Texture { int w = 0, h = 0; std::unordered_map<GLenum,GLint> params; };
std::unordered_map<GLuint,Texture> textures;
std::unordered_map<GLuint,GLuint> colors;
struct Sampler { std::unordered_map<GLenum,GLint> params; };
std::vector<Sampler> samplers(1);
GLuint bound_texture() { GLint n; glGetIntegerv(GL_TEXTURE_BINDING_2D, &n); return (GLuint)n; }
void warn(const char* s) { fprintf(stderr,"[gles] %s\n",s); }
void replace_all(std::string& s, const std::string& a, const std::string& b) {
    size_t p = 0; while ((p=s.find(a,p)) != std::string::npos) { s.replace(p,a.size(),b); p += b.size(); }
}
GLuint compile(GLenum kind, const char* source) {
    GLuint s=glCreateShader(kind); glShaderSource(s,1,&source,nullptr); glCompileShader(s);
    GLint ok=0; glGetShaderiv(s,GL_COMPILE_STATUS,&ok);
    if (!ok) { char log[2048]; glGetShaderInfoLog(s,sizeof log,nullptr,log); fprintf(stderr,"[gles] helper shader: %s\n",log); std::abort(); }
    return s;
}
}

void esGenVertexArrays(GLsizei n, GLuint* ids) { while(n--) { *ids++=(GLuint)vaos.size(); vaos.emplace_back(); } }
void esBindVertexArray(GLuint id) {
    // ES2 has no VAOs on the legacy VC4 driver. Replay the eight saved attributes.
    if (id >= vaos.size()) std::abort();
    if(id==vao) return;
    GLint old=0; glGetIntegerv(GL_ARRAY_BUFFER_BINDING,&old);
    if (vao < vaos.size()) { GLint ib; glGetIntegerv(GL_ELEMENT_ARRAY_BUFFER_BINDING,&ib); vaos[vao].indices=ib; }
    vao=id;
    for (GLuint i=0;i<8;i++) {
        const Attr& a=vaos[id].attrs[i];
        if(a.enabled) { glBindBuffer(GL_ARRAY_BUFFER,a.buffer); glVertexAttribPointer(i,a.size,a.type,a.norm,a.stride,a.ptr); glEnableVertexAttribArray(i); }
        else glDisableVertexAttribArray(i);
    }
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,vaos[id].indices);
    glBindBuffer(GL_ARRAY_BUFFER,old);
}
void esVertexAttribPointer(GLuint i, GLint sz, GLenum ty, GLboolean norm, GLsizei stride, const void* ptr) {
    if(i>=8) { warn("vertex attribute exceeds hardware limit"); std::abort(); }
    GLint b; glGetIntegerv(GL_ARRAY_BUFFER_BINDING,&b);
    Attr& a=vaos[vao].attrs[i]; a.size=sz; a.type=ty; a.norm=norm; a.stride=stride; a.ptr=ptr; a.buffer=b;
    glVertexAttribPointer(i,sz,ty,norm,stride,ptr);
}
void esEnableVertexAttribArray(GLuint i) { if(i>=8) std::abort(); vaos[vao].attrs[i].enabled=true; glEnableVertexAttribArray(i); }

void esBindFramebuffer(GLenum target, GLuint id) {
    if(target==GL_READ_FRAMEBUFFER) { read_fbo=id; return; }
    draw_fbo=id; if(target==GL_FRAMEBUFFER) read_fbo=id;
    glBindFramebuffer(GL_FRAMEBUFFER,id);
}
void esFramebufferTexture2D(GLenum target, GLenum attachment, GLenum textarget, GLuint tex, GLint level) {
    GLuint f=target==GL_READ_FRAMEBUFFER ? read_fbo:draw_fbo;
    if(attachment==GL_COLOR_ATTACHMENT0) colors[f]=tex;
    glBindFramebuffer(GL_FRAMEBUFFER,f);
    glFramebufferTexture2D(GL_FRAMEBUFFER,attachment,textarget,tex,level);
    glBindFramebuffer(GL_FRAMEBUFFER,draw_fbo);
}
void esGetIntegerv(GLenum name, GLint* value) {
    if(name==GL_READ_FRAMEBUFFER_BINDING) *value=read_fbo;
    else if(name==GL_DRAW_FRAMEBUFFER_BINDING) *value=draw_fbo;
    else if(name==GL_VERTEX_ARRAY_BINDING) *value=vao;
    else if(name==GL_MAX_SAMPLES) *value=1;
    else glGetIntegerv(name,value);
}
void esReadPixels(GLint x, GLint y, GLsizei w, GLsizei h, GLenum fmt, GLenum ty, void* dst) {
    glBindFramebuffer(GL_FRAMEBUFFER,read_fbo); glReadPixels(x,y,w,h,fmt,ty,dst); glBindFramebuffer(GL_FRAMEBUFFER,draw_fbo);
}
void esTexImage2D(GLenum target, GLint level, GLint internal, GLsizei w, GLsizei h, GLint border, GLenum fmt, GLenum ty, const void* data) {
    if(level==0) { auto& t=textures[bound_texture()]; t.w=w; t.h=h; }
    glTexImage2D(target,level,fmt,w,h,border,fmt,ty,data);
}
void esTexParameteri(GLenum target, GLenum name, GLint val) {
    if(name==GL_TEXTURE_BASE_LEVEL || name==GL_TEXTURE_MAX_LEVEL || name==GL_TEXTURE_COMPARE_MODE) return;
    textures[bound_texture()].params[name]=val; glTexParameteri(target,name,val);
}
void esGenSamplers(GLsizei n, GLuint* ids) { while(n--) { *ids++=samplers.size(); samplers.emplace_back(); } }
void esSamplerParameteri(GLuint s, GLenum p, GLint v) { samplers[s].params[p]=v; }
void esSamplerParameterf(GLuint, GLenum p, GLfloat v) {
    if(p==GL_TEXTURE_LOD_BIAS && v!=0) { static bool once=false; if(!once) { warn("sampler LOD bias is not available in legacy ES2"); once=true; } }
}
void esBindSampler(GLuint unit, GLuint s) {
    GLint active; glGetIntegerv(GL_ACTIVE_TEXTURE,&active); glActiveTexture(GL_TEXTURE0+unit);
    const auto& params=s ? samplers[s].params:textures[bound_texture()].params;
    for(auto [p,v]:params) glTexParameteri(GL_TEXTURE_2D,p,v);
    glActiveTexture(active);
}
void esEnable(GLenum cap) { if(cap!=GL_DEPTH_CLAMP && cap!=GL_COLOR_LOGIC_OP) glEnable(cap); }
void esDisable(GLenum cap) { if(cap!=GL_DEPTH_CLAMP && cap!=GL_COLOR_LOGIC_OP) glDisable(cap); }
void esLogicOp(GLenum op) {
    if(op!=GL_COPY) { static bool once=false; if(!once) { warn("non-COPY logic operation is not supported by legacy ES2"); once=true; } }
}
void esRenderbufferStorageMultisample(GLenum target, GLsizei, GLenum fmt, GLsizei w, GLsizei h) { glRenderbufferStorage(target,fmt,w,h); }

void esShaderSource(GLuint shader, GLsizei count, const GLchar* const* parts, const GLint* lengths) {
    std::string src; for(int i=0;i<count;i++) src.append(parts[i],lengths && lengths[i]>=0 ? lengths[i]:std::strlen(parts[i]));
    GLint type; glGetShaderiv(shader,GL_SHADER_TYPE,&type);
    replace_all(src,"#version 330 core", "#version 100\n#ifdef GL_FRAGMENT_PRECISION_HIGH\nprecision highp float;\n#else\nprecision mediump float;\n#endif\nprecision mediump int;");
    // Explicit attribute locations are bound before linking below.
    for(int i=0;i<16;i++) replace_all(src,"layout(location="+std::to_string(i)+") ","");
    if(type==GL_VERTEX_SHADER) {
        replace_all(src,"out vec2 ","out mediump vec2 "); replace_all(src,"out vec4 ","out mediump vec4 ");
        replace_all(src,"in ","attribute "); replace_all(src,"out ","varying ");
    }
    else {
        replace_all(src,"in vec2 ","in mediump vec2 "); replace_all(src,"in vec4 ","in mediump vec4 ");
        replace_all(src,"in ","varying ");
        replace_all(src,"out vec4 o_color;", ""); replace_all(src,"o_color =", "gl_FragColor =");
        replace_all(src,"out vec4 o;", ""); replace_all(src,"o =", "gl_FragColor =");
    }
    replace_all(src,"texture(","texture2D(");
    const char* p=src.c_str(); glShaderSource(shader,1,&p,nullptr);
}
void esLinkProgram(GLuint program) {
    // Attributes for the packed GX vertices, copy quad, and UI.
    glBindAttribLocation(program,0,"a_pos"); glBindAttribLocation(program,1,"a_colors");
    for(int i=0;i<6;i++) glBindAttribLocation(program,2+i, ("a_tcpack"+std::to_string(i)).c_str());
    glBindAttribLocation(program,0,"p"); glBindAttribLocation(program,1,"t"); glBindAttribLocation(program,2,"c");
    glBindAttribLocation(program,0,"a_quad");
    glLinkProgram(program);
    GLint ok; glGetProgramiv(program,GL_LINK_STATUS,&ok);
    if(!ok) { char log[4096]; glGetProgramInfoLog(program,sizeof log,nullptr,log); fprintf(stderr,"[gles] program link failed: %s\n",log); std::abort(); }
}
void esDrawQuad() {
    static GLuint v=0, b=0;
    if(!v) {
        const float p[]={-1,-1,1,-1,-1,1,1,1};
        esGenVertexArrays(1,&v); esBindVertexArray(v); glGenBuffers(1,&b); glBindBuffer(GL_ARRAY_BUFFER,b);
        glBufferData(GL_ARRAY_BUFFER,sizeof p,p,GL_STATIC_DRAW); esVertexAttribPointer(0,2,GL_FLOAT,GL_FALSE,0,nullptr); esEnableVertexAttribArray(0);
    }
    esBindVertexArray(v); glDrawArrays(GL_TRIANGLE_STRIP,0,4);
}
void esBlitFramebuffer(GLint sx0, GLint sy0, GLint sx1, GLint sy1, GLint dx0, GLint dy0, GLint dx1, GLint dy1, GLbitfield mask, GLenum filter) {
    if(mask!=GL_COLOR_BUFFER_BIT) { warn("depth framebuffer blits require a separate depth path"); std::abort(); }
    GLuint tex=colors[read_fbo];
    if(!tex || !textures[tex].w || !textures[tex].h) { warn("blit source has no color texture"); std::abort(); }
    static GLuint program=0;
    static GLint rect=-1, sampler=-1;
    if(!program) {
        const char* vs="attribute vec2 a_quad; varying mediump vec2 uv; uniform vec4 u_rect; void main(){ gl_Position=vec4(a_quad,0.0,1.0); uv=u_rect.xy+(a_quad*0.5+0.5)*u_rect.zw; }";
        const char* fs="precision mediump float; varying mediump vec2 uv; uniform sampler2D u_src; void main(){ gl_FragColor=texture2D(u_src,uv); }";
        program=glCreateProgram(); glAttachShader(program,compile(GL_VERTEX_SHADER,vs)); glAttachShader(program,compile(GL_FRAGMENT_SHADER,fs)); esLinkProgram(program);
        rect=glGetUniformLocation(program,"u_rect"); sampler=glGetUniformLocation(program,"u_src");
    }
    GLint prog, viewport[4], active, texture, buffer;
    glGetIntegerv(GL_CURRENT_PROGRAM,&prog); glGetIntegerv(GL_VIEWPORT,viewport); glGetIntegerv(GL_ACTIVE_TEXTURE,&active); glGetIntegerv(GL_ARRAY_BUFFER_BINDING,&buffer);
    GLuint old_vao=vao;
    GLboolean blend=glIsEnabled(GL_BLEND), depth=glIsEnabled(GL_DEPTH_TEST), cull=glIsEnabled(GL_CULL_FACE), scissor=glIsEnabled(GL_SCISSOR_TEST), cm[4];
    glGetBooleanv(GL_COLOR_WRITEMASK,cm);
    glActiveTexture(GL_TEXTURE0); glGetIntegerv(GL_TEXTURE_BINDING_2D,&texture); glBindTexture(GL_TEXTURE_2D,tex);
    GLint minf,magf,wraps,wrapt;
    glGetTexParameteriv(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,&minf); glGetTexParameteriv(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,&magf);
    glGetTexParameteriv(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,&wraps); glGetTexParameteriv(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,&wrapt);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,filter); glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,filter);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE); glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE);
    if(dx1<dx0) { std::swap(dx0,dx1); std::swap(sx0,sx1); }
    if(dy1<dy0) { std::swap(dy0,dy1); std::swap(sy0,sy1); }
    const auto& t=textures[tex];
    glBindFramebuffer(GL_FRAMEBUFFER,draw_fbo); glViewport(dx0,dy0,dx1-dx0,dy1-dy0);
    glDisable(GL_BLEND); glDisable(GL_DEPTH_TEST); glDisable(GL_CULL_FACE); glDisable(GL_SCISSOR_TEST); glColorMask(1,1,1,1);
    glUseProgram(program); glUniform1i(sampler,0); glUniform4f(rect,(float)sx0/t.w,(float)sy0/t.h,(float)(sx1-sx0)/t.w,(float)(sy1-sy0)/t.h);
    esDrawQuad();
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,minf); glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,magf);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,wraps); glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,wrapt);
    glBindTexture(GL_TEXTURE_2D,texture); glActiveTexture(active); glUseProgram(prog); esBindVertexArray(old_vao); glBindBuffer(GL_ARRAY_BUFFER,buffer);
    glViewport(viewport[0],viewport[1],viewport[2],viewport[3]); glColorMask(cm[0],cm[1],cm[2],cm[3]);
    if(blend) glEnable(GL_BLEND); if(depth) glEnable(GL_DEPTH_TEST); if(cull) glEnable(GL_CULL_FACE); if(scissor) glEnable(GL_SCISSOR_TEST);
}
#endif
