// TEV -> GLSL fragment shader generation.
#include "gx_render.h"
#include <string>
#include <sstream>
#include <unordered_map>
#include <cstdio>

static const char* kColorIn[16] = {"prev.rgb", "prev.aaa", "c0.rgb", "c0.aaa", "c1.rgb", "c1.aaa", "c2.rgb", "c2.aaa",
                                   "tex.rgb", "tex.aaa", "ras.rgb", "ras.aaa", "vec3(1.0)", "vec3(0.5)", "kc", "vec3(0.0)"};
static const char* kAlphaIn[8] = {"prev.a", "c0.a", "c1.a", "c2.a", "tex.a", "ras.a", "ka", "0.0"};
static const char* kDest[4] = {"prev", "c0", "c1", "c2"};
static const char* kKFrac[8] = {"1.0", "0.875", "0.75", "0.625", "0.5", "0.375", "0.25", "0.125"};

static std::string kcolor(int sel) {
    if (sel < 8) return std::string("vec3(") + kKFrac[sel] + ")";
    if (sel >= 12 && sel <= 15) return "u_konst[" + std::to_string(sel - 12) + "].rgb";
    if (sel >= 16) { int k = (sel - 16) & 3, c = (sel - 16) >> 2; return std::string("vec3(u_konst[") + std::to_string(k) + "]." + "rgba"[c] + ")"; }
    return "vec3(1.0)";
}
static std::string kalpha(int sel) {
    if (sel < 8) return kKFrac[sel];
    if (sel >= 16) { int k = (sel - 16) & 3, c = (sel - 16) >> 2; return std::string("u_konst[") + std::to_string(k) + "]." + "rgba"[c]; }
    return "1.0";
}
static std::string swizzle(const uint32_t* bp, int table) {
    uint32_t a = bp[0xF6 + 2 * table], b = bp[0xF6 + 2 * table + 1];
    std::string s = ".";
    s += "rgba"[a & 3]; s += "rgba"[(a >> 2) & 3]; s += "rgba"[b & 3]; s += "rgba"[(b >> 2) & 3];
    return s;
}

static const char* kAlphaCmp[8] = {"false", "(A < R)", "(A == R)", "(A <= R)", "(A > R)", "(A != R)", "(A >= R)", "true"};

#ifdef USE_GLES2
namespace {
struct Interface { unsigned coords=0,textures=0,colors=0; bool packed=false; };
Interface shader_interface(const uint32_t* bp) {
    Interface used;
    int stages=((bp[0]>>10)&15)+1;
    for(int s=0;s<stages;s++) {
        uint32_t order=bp[0x28+s/2]>>((s&1)*12);
        if(order&64) { used.coords|=1u<<((order>>3)&7); used.textures|=1u<<(order&7); }
        int channel=(order>>7)&7; if(channel<2) used.colors|=1u<<channel;
    }
    used.packed=__builtin_popcount(used.coords)+__builtin_popcount(used.colors)>8;
    return used;
}
const char* tc_expr[8]={"a_tcpack0.xyz","vec3(a_tcpack0.w,a_tcpack1.xy)","vec3(a_tcpack1.zw,a_tcpack2.x)","a_tcpack2.yzw",
    "a_tcpack3.xyz","vec3(a_tcpack3.w,a_tcpack4.xy)","vec3(a_tcpack4.zw,a_tcpack5.x)","a_tcpack5.yzw"};
}
std::string tev_vertex_shader_source(const uint32_t* bp) {
    Interface used=shader_interface(bp);std::ostringstream o;
    o<<"#version 330 core\nprecision highp float; in vec4 a_pos;\n";
    if(used.colors) o<<"in vec4 a_colors;\n";
    unsigned inputs=0;
    for(int t=0;t<8;t++) if(used.coords&(1u<<t)) { inputs|=1u<<((3*t)/4); inputs|=1u<<((3*t+2)/4); }
    if(used.packed) inputs=63;
    for(int i=0;i<6;i++) if(inputs&(1u<<i)) o<<"in vec4 a_tcpack"<<i<<";\n";
    if(used.packed) o<<"out mediump vec4 v_tcpack[6];\n";
    else for(int t=0;t<8;t++) if(used.coords&(1u<<t)) o<<"out mediump vec3 v_tc"<<t<<";\n";
    for(int c=0;c<2;c++) if(used.colors&(1u<<c)) o<<"out mediump vec4 v_c"<<c<<";\n";
    o<<"void main(){gl_Position=a_pos;gl_Position.z=clamp(a_pos.z,-a_pos.w,a_pos.w);\n";
    if(used.colors&1) o<<"v_c0=mod(a_colors,256.0)/255.0;\n";
    if(used.colors&2) o<<"v_c1=floor(a_colors/256.0)/255.0;\n";
    if(used.packed) for(int i=0;i<6;i++) o<<"v_tcpack["<<i<<"]=a_tcpack"<<i<<";\n";
    else for(int t=0;t<8;t++) if(used.coords&(1u<<t)) o<<"v_tc"<<t<<"="<<tc_expr[t]<<";\n";
    o<<"}\n";return o.str();
}
#endif

std::string tev_shader_source(const uint32_t* bp) {
    int nstages = ((bp[0x00] >> 10) & 15) + 1;
    std::ostringstream o;
#ifdef USE_GLES2
    Interface used=shader_interface(bp);
    o<<"#version 330 core\n";
    for(int c=0;c<2;c++) if(used.colors&(1u<<c)) o<<"in mediump vec4 v_c"<<c<<";\n";
    if(used.packed) o<<"in mediump vec4 v_tcpack[6];\n";
    else for(int t=0;t<8;t++) if(used.coords&(1u<<t)) o<<"in mediump vec3 v_tc"<<t<<";\n";
    for(int t=0;t<8;t++) if(used.textures&(1u<<t)) o<<"uniform sampler2D u_tex"<<t<<";\n";
    o<<"out vec4 o_color; uniform vec4 u_reg[4]; uniform vec4 u_konst[4]; uniform vec2 u_aref;\n"
       "vec3 q8(vec3 v){return floor(v*255.0+0.5);} float q8(float v){return floor(v*255.0+0.5);}\nvoid main(){\n";
    if(used.packed) o<<"vec3 v_tc[8];\n"
         "v_tc[0]=v_tcpack[0].xyz; v_tc[1]=vec3(v_tcpack[0].w,v_tcpack[1].xy);\n"
         "v_tc[2]=vec3(v_tcpack[1].zw,v_tcpack[2].x); v_tc[3]=v_tcpack[2].yzw;\n"
         "v_tc[4]=v_tcpack[3].xyz; v_tc[5]=vec3(v_tcpack[3].w,v_tcpack[4].xy);\n"
         "v_tc[6]=vec3(v_tcpack[4].zw,v_tcpack[5].x); v_tc[7]=v_tcpack[5].yzw;\n";
    o<<"vec4 prev=u_reg[0],c0=u_reg[1],c1=u_reg[2],c2=u_reg[3]; vec4 tex=vec4(1.0),ras=vec4(0.0); vec3 kc; float ka;\n";
#else
    o << "#version 330 core\n"
         "in vec4 v_c0; in vec4 v_c1; in vec3 v_tc[8];\n"
         "out vec4 o_color;\n"
         "uniform sampler2D u_tex[8];\n"
         "uniform vec4 u_reg[4];\n"
         "uniform vec4 u_konst[4];\n"
         "uniform vec2 u_aref;\n"
         "vec3 q8(vec3 v){ return floor(v*255.0+0.5); }\n"
         "float q8(float v){ return floor(v*255.0+0.5); }\n"
         "void main(){\n"
         "  vec4 prev=u_reg[0], c0=u_reg[1], c1=u_reg[2], c2=u_reg[3];\n"
         "  vec4 tex=vec4(1.0), ras=vec4(0.0); vec3 kc; float ka;\n";
#endif
    for (int s = 0; s < nstages; s++) {
        uint32_t order = bp[0x28 + s / 2] >> ((s & 1) * 12);
        int texmap = order & 7, texcoord = (order >> 3) & 7, enable = (order >> 6) & 1, chan = (order >> 7) & 7;
        uint32_t cenv = bp[0xC0 + 2 * s], aenv = bp[0xC1 + 2 * s];
        uint32_t ksel = bp[0xF6 + s / 2] >> ((s & 1) ? 14 : 4);
        int kcs = ksel & 31, kas = (ksel >> 5) & 31;
        int rswap = aenv & 3, tswap = (aenv >> 2) & 3;
        o << "  // stage " << s << "\n";
        if (enable) {
#ifdef USE_GLES2
            std::string tc=used.packed?"v_tc["+std::to_string(texcoord)+"]":"v_tc"+std::to_string(texcoord);
            o<<"tex=texture(u_tex"<<texmap<<", "<<tc<<".xy/("<<tc<<".z==0.0?1.0:"<<tc<<".z))"<<swizzle(bp,tswap)<<";\n";
#else
            o << "  tex = texture(u_tex[" << texmap << "], v_tc[" << texcoord << "].xy / (v_tc[" << texcoord << "].z == 0.0 ? 1.0 : v_tc[" << texcoord << "].z))" << swizzle(bp, tswap) << ";\n";
#endif
        } else
            o << "  tex = vec4(1.0);\n";
        if (chan == 0) o << "  ras = v_c0" << swizzle(bp, rswap) << ";\n";
        else if (chan == 1) o << "  ras = v_c1" << swizzle(bp, rswap) << ";\n";
        else o << "  ras = vec4(0.0);\n";
        o << "  kc = " << kcolor(kcs) << "; ka = " << kalpha(kas) << ";\n";
        // color combiner
        {
            const char *d = kColorIn[cenv & 15], *c = kColorIn[(cenv >> 4) & 15], *b = kColorIn[(cenv >> 8) & 15], *a = kColorIn[(cenv >> 12) & 15];
            int bias = (cenv >> 16) & 3, op = (cenv >> 18) & 1, clamp = (cenv >> 19) & 1, scale = (cenv >> 20) & 3, dest = (cenv >> 22) & 3;
            std::string r;
            if (bias != 3) {
                static const char* kb[3] = {"0.0", "0.5", "-0.5"};
                static const char* ks[4] = {"1.0", "2.0", "4.0", "0.5"};
                r = std::string("((") + d + (op ? " - " : " + ") + "mix(" + a + ", " + b + ", " + c + ") + vec3(" + kb[bias] + ")) * " + ks[scale] + ")";
            } else {
                int mode = (scale << 1) | op;
                std::string A = a, B = b;
                switch (mode) {
                    case 0: r = std::string(d) + " + ((q8((" + A + ").r) > q8((" + B + ").r)) ? " + c + " : vec3(0.0))"; break;
                    case 1: r = std::string(d) + " + ((q8((" + A + ").r) == q8((" + B + ").r)) ? " + c + " : vec3(0.0))"; break;
                    case 2: r = std::string(d) + " + ((dot(q8((" + A + ").rg), vec2(1.0,256.0)) > dot(q8((" + B + ").rg), vec2(1.0,256.0))) ? " + c + " : vec3(0.0))"; break;
                    case 3: r = std::string(d) + " + ((dot(q8((" + A + ").rg), vec2(1.0,256.0)) == dot(q8((" + B + ").rg), vec2(1.0,256.0))) ? " + c + " : vec3(0.0))"; break;
                    case 4: r = std::string(d) + " + ((dot(q8(" + A + "), vec3(1.0,256.0,65536.0)) > dot(q8(" + B + "), vec3(1.0,256.0,65536.0))) ? " + c + " : vec3(0.0))"; break;
                    case 5: r = std::string(d) + " + ((dot(q8(" + A + "), vec3(1.0,256.0,65536.0)) == dot(q8(" + B + "), vec3(1.0,256.0,65536.0))) ? " + c + " : vec3(0.0))"; break;
                    case 6: r = std::string(d) + " + " + c + " * vec3(greaterThan(q8(" + A + "), q8(" + B + ")))"; break;
                    default: r = std::string(d) + " + " + c + " * vec3(equal(q8(" + A + "), q8(" + B + ")))"; break;
                }
            }
            o << "  " << kDest[dest] << ".rgb = " << (clamp ? "clamp(" + r + ", 0.0, 1.0)" : "clamp(" + r + ", -4.0, 4.0)") << ";\n";
        }
        // alpha combiner
        {
            const char *d = kAlphaIn[(aenv >> 4) & 7], *c = kAlphaIn[(aenv >> 7) & 7], *b = kAlphaIn[(aenv >> 10) & 7], *a = kAlphaIn[(aenv >> 13) & 7];
            int bias = (aenv >> 16) & 3, op = (aenv >> 18) & 1, clamp = (aenv >> 19) & 1, scale = (aenv >> 20) & 3, dest = (aenv >> 22) & 3;
            std::string r;
            if (bias != 3) {
                static const char* kb[3] = {"0.0", "0.5", "-0.5"};
                static const char* ks[4] = {"1.0", "2.0", "4.0", "0.5"};
                r = std::string("((") + d + (op ? " - " : " + ") + "mix(" + a + ", " + b + ", " + c + ") + " + kb[bias] + ") * " + ks[scale] + ")";
            } else {
                int mode = (scale << 1) | op;
                const char* cmp = (mode & 1) ? " == " : " > ";
                r = std::string(d) + " + ((q8(" + a + ")" + cmp + "q8(" + b + ")) ? " + c + " : 0.0)";
            }
            o << "  " << kDest[dest] << ".a = " << (clamp ? "clamp(" + r + ", 0.0, 1.0)" : "clamp(" + r + ", -4.0, 4.0)") << ";\n";
        }
    }
    // alpha test
    uint32_t ac = bp[0xF3];
    int comp0 = (ac >> 16) & 7, comp1 = (ac >> 19) & 7, logic = (ac >> 22) & 3;
    auto cmp = [&](int f, const char* ref) {
        std::string s = kAlphaCmp[f];
        size_t p;
        while ((p = s.find('A')) != std::string::npos) s.replace(p, 1, "pa");
        while ((p = s.find('R')) != std::string::npos) s.replace(p, 1, ref);
        return s;
    };
    o << "  vec4 outc = clamp(prev, 0.0, 1.0);\n";
    if (!(comp0 == 7 && comp1 == 7 && logic == 0)) {
        static const char* kl[4] = {" && ", " || ", " != ", " == "};
        o << "  float pa = q8(outc.a);\n"
          << "  bool t0 = " << cmp(comp0, "u_aref.x") << "; bool t1 = " << cmp(comp1, "u_aref.y") << ";\n"
          << "  if (!(t0" << kl[logic] << "t1)) discard;\n";
    }
    o << "  o_color = outc;\n}\n";
    return o.str();
}

uint64_t tev_shader_key(const uint32_t* bp) {
    int nstages = ((bp[0x00] >> 10) & 15) + 1;
    uint64_t h = 1469598103934665603ull;
    auto mix = [&](uint32_t v) { h ^= v; h *= 1099511628211ull; };
    mix(nstages);
    for (int s = 0; s < nstages; s++) {
        mix(bp[0xC0 + 2 * s]); mix(bp[0xC1 + 2 * s]);
        mix(bp[0x28 + s / 2]);
    }
    for (int i = 0; i < 8; i++) mix(bp[0xF6 + i]);
    mix(bp[0xF3] >> 16);
    return h;
}
