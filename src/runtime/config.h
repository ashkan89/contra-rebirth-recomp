#pragma once
#include <string>

// Settings from the .ini file (next to the executable or in the working folder).
// Every key can also be overridden with an environment variable of the same name in
// upper case (e.g. RESOLUTION_SCALE=3).
struct Config {
    int render_resolution = 0;        // 0 = Wii scale below, 1 = 320x280, 2 = 480x320
#ifdef PI_ZERO2
    int resolution_scale = 1;
#else
    int resolution_scale = 2;          // internal render resolution = 640x528 * scale
#endif
    bool vsync = true;
    bool widescreen = true;            // tell the game the TV is 16:9 and present at 16:9
#ifdef PI_ZERO2
    bool fullscreen = true;
    int window_scale = 1;
#else
    bool fullscreen = false;
    int window_scale = 2;              // initial window size = 640x480 * window_scale (or 854x480 widescreen)
#endif
    std::string controls = "sideways"; // "sideways", "nunchuk" or "classic" (Classic Controller)
    bool show_perf = true;
#ifdef PI_ZERO2
    int anisotropy = 1;
#else
    int anisotropy = 16;               // anisotropic texture filtering: 1 (off), 2, 4, 8, 16
#endif
    bool smooth_textures = true;       // trilinear mipmaps (generated when the game has none)
    bool swap_ab_xy = false;           // gamepad: swap A<->B and X<->Y (Nintendo-style positions)
    int speed = 1;                     // game speed multiplier 1..8 (fast-forward)
#ifdef PI_ZERO2
    int msaa = 1;
#else
    int msaa = 4;                      // anti-aliasing samples: 1 (off), 2, 4, 8
#endif
    int render_width() const { return render_resolution == 1 ? 320 : render_resolution == 2 ? 480 : 640 * resolution_scale; }
    int render_height() const { return render_resolution == 1 ? 280 : render_resolution == 2 ? 320 : 528 * resolution_scale; }
};
extern Config g_cfg;
void config_load(const char* exe_path);
void config_save();
