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

// the languages, saved by these codes: English, French, German, Italian, Spanish, Indonesian. Indonesian is the
// recomp's own text only: the game files have none, so the game itself stays in English
inline constexpr const char* kLanguages[] = {"int", "fra", "deu", "ita", "esp", "ind"};
inline bool KnownLanguage(const std::string& code) {
    return std::find(std::begin(kLanguages), std::end(kLanguages), code) != std::end(kLanguages);
}

// SSAA renders at twice the height, the presenter averages it down
inline constexpr uint32_t kAaOff = 0, kAaFxaa = 1, kAaSsaa = 2;

// time of day: sky colour, sun colour, tint, fog and whether the headlights are on
inline constexpr uint32_t kTimeMorning = 0, kTimeDay = 1, kTimeAfternoon = 2, kTimeNight = 3,
                            kTimeMidnight = 4, kTimeDawn = 5, kTimeAuto = 6;

// The concrete time of day actually used for lighting: when the player picks AUTO this follows the real-world
// clock (Dawn 5-7, Morning 7-10, Day 10-16, Afternoon 16-19, Night 19-22, Midnight otherwise).
// ApplyGraphicsSwitches() re-resolves it once per frame; everything else reads the cache.
uint32_t CurrentTimeOfDay();

// headlights: AUTO follows the time of day (Night and Midnight), ON and OFF force them whatever the sky does
inline constexpr uint32_t kHeadlightsAuto = 0, kHeadlightsOn = 1, kHeadlightsOff = 2;

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
    uint32_t time_of_day = kTimeDay;
    uint32_t headlights = kHeadlightsAuto;
    bool music_restored = false;
    bool shader_preload = false;
    bool texture_packs = true;
    uint32_t texture_resolution = 0;
    bool motion_blur = true;
    uint32_t headlight_width = 100;
    uint32_t headlight_brightness = 100;
    uint32_t headlight_color = 0;
    uint32_t headlight_distance = 100;
    uint32_t headlight_height = 100;
    bool headlight_adaptive = false;
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
inline bool NightMode() {
    uint32_t tod = CurrentTimeOfDay();
    return tod == kTimeNight || tod == kTimeMidnight;
}
inline bool SunUp() {
    uint32_t tod = CurrentTimeOfDay();
    return tod == kTimeDawn || tod == kTimeMorning || tod == kTimeDay || tod == kTimeAfternoon;
}
inline bool HeadlightsOn() {
    switch (Opt().headlights) {
        case kHeadlightsOn:  return true;
        case kHeadlightsOff: return false;
        case kHeadlightsAuto:
        default:             return NightMode();
    }
}

// ---- rain ----
// 0 dry .. 255 fully wet. While Opt().rain is on in a race, rises in half a second; after it stops — or the
// player leaves the race, opens a replay or a cutscene — decays over three seconds.
inline std::atomic<uint8_t> g_rain_wetness{0};
// true while the camera is inside a tunnel: rain and every wet look are hidden inside, so the road does not
// glisten under a ceiling. RainWetness() returns 0 while it is set, but g_rain_wetness keeps tracking the
// real rain underneath, so leaving the tunnel shows the wet road again instantly.
inline std::atomic<bool> g_in_tunnel{false};
inline uint8_t RainWetness() {
    if (g_in_tunnel.load(std::memory_order_relaxed)) return 0;
    return g_rain_wetness.load(std::memory_order_relaxed);
}

// Opt().rain, racing, no cinematic or replay, and not inside a tunnel: whether rain is being fed right now.
bool Raining();

// Advances RainWetness toward the rain's own state (ignoring the tunnel), so g_rain_wetness is already 255
// when the player leaves a tunnel. Called once a frame from ApplyGraphicsSwitches, first thing.
void UpdateRainWetness();

inline void TimeOfDayLight(float& r, float& g, float& b) {
    switch (CurrentTimeOfDay()) {
        case kTimeMorning:   r = 1.02f; g = 1.01f; b = 0.96f; break;
        case kTimeAfternoon: r = 1.20f; g = 0.90f; b = 0.68f; break;
        case kTimeNight:     r = 0.42f; g = 0.52f; b = 0.72f; break;
        case kTimeMidnight:  r = 0.20f; g = 0.28f; b = 0.48f; break;
        case kTimeDawn:      r = 0.98f; g = 0.84f; b = 0.90f; break;
        case kTimeDay:
        default:             r = g = b = 1.0f; break;
    }
}

inline void TimeOfDayAmbient(float& r, float& g, float& b) {
    switch (CurrentTimeOfDay()) {
        case kTimeMorning:   r = 1.04f; g = 1.02f; b = 0.98f; break;
        case kTimeAfternoon: r = 1.12f; g = 0.94f; b = 0.82f; break;
        case kTimeNight:     r = 0.38f; g = 0.48f; b = 0.70f; break;
        case kTimeMidnight:  r = 0.20f; g = 0.26f; b = 0.45f; break;
        case kTimeDawn:      r = 1.00f; g = 0.88f; b = 0.92f; break;
        case kTimeDay:
        default:             r = g = b = 1.0f; break;
    }
}

// the camera's forward direction and world position, written by the renderer each draw; read by the sky overlay
inline std::atomic<float> g_cam_forward_x{0.0f};
inline std::atomic<float> g_cam_forward_y{0.0f};
inline std::atomic<float> g_cam_forward_z{1.0f};
inline std::atomic<float> g_cam_x{0.0f};
inline std::atomic<float> g_cam_y{0.0f};
inline std::atomic<float> g_cam_z{0.0f};
inline std::atomic<bool>  g_cam_valid{false};

// the sky overlay: the sun and its corona through Dawn/Morning/Day/Afternoon; stars, the moon, shooting stars at
// Midnight (dimmed stars at Dawn); the rain's screen droplets, streaks and cool wet haze whenever RainWetness()
// is above a few. Called from app::DrawUi.
void DrawSkyElements();

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
