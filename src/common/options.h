#pragma once
#include <algorithm>
#include <atomic>
#include <cstdint>
#include <iterator>
#include <map>
#include <string>

struct X86Context;
namespace input {
struct Pad;
}

// the game's own settings, in the settings file through app::kGame.options
namespace game {

inline constexpr const char* kLanguages[] = {"int", "fra", "deu", "ita", "esp", "ind"};
inline bool KnownLanguage(const std::string& code) {
    return std::find(std::begin(kLanguages), std::end(kLanguages), code) != std::end(kLanguages);
}

inline constexpr uint32_t kAaOff = 0, kAaFxaa = 1, kAaSsaa = 2;

struct Options {
    bool fps_counter = false;
    uint32_t fps_position = 0;
    bool fps_usage = true;
    bool fps_temps = true;
    uint32_t screen_filter = 0;
    uint32_t anti_aliasing = 0;
    uint32_t aspect_ratio = 0;
    bool bloom = true;
    bool fog = true;
    uint32_t reflections = 2;
    bool particles = true;
    bool shadows = true;
    bool road_reflections = true;
    bool rain = true;
    bool rain_drops = true;
    bool yellow_tint = false;
    bool night_mode = false;
    bool music_restored = false;
    bool shader_preload = false;
    bool texture_packs = true;
    uint32_t texture_resolution = 0;
    bool motion_blur = true;
    // headlight (night mode only): width, brightness, colour, distance, height
    uint32_t headlight_width = 100;       // 50-600 %, multiplies the beam quad's width
    uint32_t headlight_brightness = 100;  // 50-600 %, multiplies the beam colour's alpha
    uint32_t headlight_color = 0;         // preset index (0-15), 0 = warm white
    uint32_t headlight_distance = 100;    // 50-600 %, multiplies the beam's forward reach
    uint32_t headlight_height = 100;      // 50-600 %, multiplies the beam's centre distance
    // cheats
    bool cheat_nos = false;
    bool cheat_speedbreaker = false;
    bool cheat_cash = false;
    bool cheat_no_police = false;
    bool cheat_unlock_all = false;
    bool cheat_no_traffic = false;
    bool cheat_drag_shift = false;
    bool skip_movies = false;
    uint32_t audio_output = 0;
    std::string language = "int";
    std::map<std::string, std::string> keys;
    std::map<std::string, std::string> pad;
};
Options& Opt();
const std::string& BootLanguage();
bool LanguageAvailable(const std::string& code);
inline std::atomic<uint32_t> g_road_reflection{0};
inline std::atomic<uint32_t> g_env_cube{0};
inline std::atomic<bool> g_focus_paused{false};
inline std::atomic<uint32_t> g_focus_pauses{0};
void WaitWhileUnfocused();
void WidenAspect();
void ApplyGraphicsSwitches();
void ApplyCheats();
bool Racing();
inline std::atomic<bool> g_reset_car{false};
void ResetCarFrame(X86Context& ctx);
void DragShiftFrame(X86Context& ctx);
void MusicFrame(X86Context& ctx);
void ModCarsFrame(X86Context& ctx);
uint32_t ModCarTexture(X86Context& ctx, uint32_t hash);
void TexturePacksFrame(X86Context& ctx);
void CinematicRequest(bool from_pause_menu = false);
bool CinematicActive();
bool CinematicClean();
bool CinematicPad(int port, const input::Pad& pad);
void DrawCinematicHud();
bool CinematicMenuHit(float x, float y);
void ReplayRequest(bool from_pause_menu = false);
bool ReplayActive();
void ReplayFrame(X86Context& ctx);
bool ReplayPad(int port, const input::Pad& pad);
void DrawReplayHud();
bool ReplayMenuHit(float x, float y);
bool ReplayStreaming();
void ReplayRaceRestarted();

}  // namespace game
