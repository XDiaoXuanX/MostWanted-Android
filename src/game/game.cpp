// Need for Speed: Most Wanted (Black Edition) needs no patch of its code to boot: the XDK it calls is all on the host.
// Its own hooks go here, [[hook]] entries in config/game.toml and X86_FUNC replacements of recompiled functions.
#include <algorithm>
#include <array>
#include <atomic>
#include <chrono>
#include <cstdint>
#include <cstring>
#include <strings.h>
#include <string>
#include <vector>
#include <unordered_map>

#include "app/app.h"
#include "common/aspect.h"
#include "common/options.h"
#include "core/log.h"
#include "cpu/x86_ops.h"
#include "common/music.h"
#include "kernel/kernel.h"
#include "kernel/memory.h"

namespace game {

constexpr uint32_t kVideoFlags = 0x48099C, kWideWidth = 0x427C00;
constexpr uint32_t kSafeWidth = 0x3D3534, kSafeHeight = 0x3D3530;
void FullFrameFrontEnd() {
    constexpr float kWidth = 640.0f, kHeight = 480.0f;
    uint32_t w, h;
    std::memcpy(&w, &kWidth, 4);
    std::memcpy(&h, &kHeight, 4);
    kernel::W32(kSafeWidth, w);
    kernel::W32(kSafeHeight, h);
}
void WidenAspect() {
    FullFrameFrontEnd();
    if (!(kernel::G<uint8_t>(kVideoFlags) & 1)) return;
    float w = 480.0f * Aspect();
    uint32_t bits;
    std::memcpy(&bits, &w, 4);
    kernel::W32(kWideWidth, bits);
}

constexpr uint32_t kViewType = 0x4, kProjection = 0x40;
uint32_t g_widen_view = 0;
bool WideView(uint32_t type) { return type == 1 || type == 2 || (type >= 4 && type <= 7); }
}  // namespace game

X86_EXTERN_FUNC(__imp__sub_000C7740);
X86_FUNC(sub_000C7740) {
    uint32_t view = kernel::R32(ctx.esp + 4);
    bool wide = (kernel::G<uint8_t>(game::kVideoFlags) & 1) && view && game::WideView(kernel::R32(view + game::kViewType));
    game::g_widen_view = wide ? ctx.ecx : 0;
    __imp__sub_000C7740(ctx, base);
    game::g_widen_view = 0;
}

X86_EXTERN_FUNC(__imp__sub_002B91C4);
X86_FUNC(sub_002B91C4) {
    uint32_t m = game::g_widen_view;
    if (m && kernel::R32(ctx.esp + 8) == m && kernel::R32(ctx.esp + 12) == m + game::kProjection) {
        game::g_widen_view = 0;
        float& x = kernel::G<float>(m + game::kProjection);
        x *= (16.0f / 9.0f) / game::Aspect();
    }
    __imp__sub_002B91C4(ctx, base);
}

namespace game {

constexpr uint32_t kEnvMapSwitch = 0x4279D6, kParticleDrawSwitch = 0x4279CF, kCarShadowSwitch = 0x469910;
constexpr uint32_t kRoadReflectionSwitch = 0x427A3C, kRoadReflectionView = 0x48AE40;
constexpr uint32_t kEnvMapFace0Target = 0x48B4C0;
constexpr uint32_t kSkyMidday = 0x42FDA0, kSkySunset = 0x42FDB0;
constexpr uint32_t kHorizonHaze = 0x4279D7;
constexpr uint32_t kHeadlightBeamColor = 0x469B0C;
inline int g_headlights_clear_frames = 0;
inline bool g_prev_night_mode = false;

uint32_t TargetData(uint32_t target) {
    uint32_t surface = target ? kernel::R32(target + 8) : 0;
    return surface ? mem::VirtualFromPhysical(kernel::R32(surface + 4)) : 0;
}

void ApplyGraphicsSwitches() {
    constexpr uint32_t kRaceWarmup = 120;
    static uint32_t frame = 0, racing = 0;
    racing = Racing() ? racing + (racing < kRaceWarmup) : 0;
    uint32_t rate = Opt().reflections;
    uint32_t every = racing < kRaceWarmup ? 1 : rate < 3 ? 1u << rate : 512;
    bool envmap = frame++ % every == 0;
    kernel::G<uint8_t>(kEnvMapSwitch) = envmap ? 1 : 0;
    kernel::G<uint8_t>(kParticleDrawSwitch) = Opt().particles ? 1 : 0;
    kernel::W32(kCarShadowSwitch, Opt().shadows && !ReplayActive() ? 1 : 0);
    kernel::W32(kRoadReflectionSwitch, Opt().road_reflections ? 1 : 0);
    g_road_reflection = TargetData(kernel::R32(kRoadReflectionView + 0x60));
    g_env_cube = TargetData(kernel::R32(kEnvMapFace0Target));

    // ---- the sky: a morning, the original day, a golden afternoon, or night ----
    static const float kSkyMorningMidday[4] = {1.08f, 1.04f, 0.98f, 1.0f};
    static const float kSkyMorningSunset[4] = {1.12f, 1.00f, 0.90f, 1.0f};
    static const float kSkyDayMidday[4]     = {0.9723f, 0.9900f, 1.1000f, 1.0f};
    static const float kSkyDaySunset[4]     = {0.95858f, 1.0700f, 1.1150f, 1.0f};
    static const float kSkyAfternoonMidday[4] = {1.18f, 1.05f, 0.85f, 1.0f};
    static const float kSkyAfternoonSunset[4] = {1.25f, 0.90f, 0.62f, 1.0f};
    static const float kSkyNight[4]         = {0.10f, 0.15f, 0.32f, 1.0f};
    const float* midday;
    const float* sunset;
    switch (Opt().time_of_day) {
        case kTimeMorning:
            midday = kSkyMorningMidday;
            sunset = kSkyMorningSunset;
            break;
        case kTimeAfternoon:
            midday = kSkyAfternoonMidday;
            sunset = kSkyAfternoonSunset;
            break;
        case kTimeNight:
            midday = sunset = kSkyNight;
            break;
        case kTimeDay:
        default:
            midday = kSkyDayMidday;
            sunset = kSkyDaySunset;
            break;
    }
    std::memcpy(kernel::HostPtr(kSkyMidday), midday, sizeof(kSkyNight));
    std::memcpy(kernel::HostPtr(kSkySunset), sunset, sizeof(kSkyNight));
    kernel::G<uint8_t>(kHorizonHaze) = Opt().time_of_day == kTimeNight ? 0 : 1;

    // ---- headlight: colour preset x brightness, and beam geometry scaled by options ----
    // Format 0xAARRGGBB. Index 0 matches the old night-mode default (0x48FFF4D8) exactly.
    // Order matches menu.cpp's kHeadlightColorNames[] (16 entries).
    static const uint32_t kHeadlightPresets[] = {
        0x48FFF4D8, 0x48FFFFFF, 0x48E0F0FF, 0x484060FF,
        0x48FFFF80, 0x48C040FF, 0x48FFC080, 0x48FF4040,
        0x48FFB020, 0x4840FF40, 0x4830E0FF, 0x48FF80D0,
        0x48B0FF30, 0x4830D0A0, 0x48FF00B4, 0x4890FFD0,
    };
    constexpr uint32_t kHeadlightPresetCount =
        sizeof(kHeadlightPresets) / sizeof(kHeadlightPresets[0]);

    const uint32_t width_pct = std::clamp<uint32_t>(Opt().headlight_width, 25u, 600u);
    const uint32_t distance_pct = std::clamp<uint32_t>(Opt().headlight_distance, 25u, 600u);
    const uint32_t height_pct = std::clamp<uint32_t>(Opt().headlight_height, 25u, 600u);
    const float width_scale = float(width_pct) / 100.0f;
    const float distance_scale = float(distance_pct) / 100.0f;
    const float height_scale = float(height_pct) / 100.0f;

    static const float kBeamDay[5] = {13.0f, 10.0f, 4.9f, 10.0f, 4.9f};
    static const float kBeamNight[5] = {10.0f, 6.0f, 5.2f, 6.0f, 2.4f};
    const float* base_beam = NightMode() ? kBeamNight : kBeamDay;
    const float beam[5] = {
        base_beam[0] * height_scale,     // center distance
        base_beam[1] * distance_scale,   // half-length (front)
        base_beam[2] * width_scale,      // width ahead
        base_beam[3] * distance_scale,   // half-length (rear)
        base_beam[4] * width_scale,      // width at bumper
    };
    kernel::G<float>(0x469ACC) = beam[0];
    kernel::G<float>(0x469AE4) = beam[1];
    kernel::G<float>(0x469AE8) = beam[1];
    kernel::G<float>(0x469AEC) = beam[2];
    kernel::G<float>(0x469AF0) = beam[2];
    kernel::G<float>(0x469AF4) = beam[3];
    kernel::G<float>(0x469AF8) = beam[3];
    kernel::G<float>(0x469AFC) = beam[4];
    kernel::G<float>(0x469B00) = beam[4];

    // Colour & brightness: night only (daytime beam stays 0x80cccccc so daylight shading is unchanged).
    uint32_t beam_color = 0x80cccccc;
    if (NightMode()) {
        const uint32_t preset = kHeadlightPresets[Opt().headlight_color % kHeadlightPresetCount];
        const uint32_t brightness_pct = std::clamp<uint32_t>(Opt().headlight_brightness, 25u, 600u);
        uint32_t alpha = (preset >> 24) & 0xFFu;
        alpha = std::min<uint32_t>(0xFFu, alpha * brightness_pct / 100u);
        beam_color = (alpha << 24) | (preset & 0x00FFFFFFu);
    }
    kernel::W32(kHeadlightBeamColor, beam_color);

    bool night = NightMode();
    if (!night && g_prev_night_mode) g_headlights_clear_frames = 120;
    g_prev_night_mode = night;
    if (g_headlights_clear_frames > 0) --g_headlights_clear_frames;
}

constexpr uint32_t kInfiniteNos = 0x49A61C, kInfiniteSpeedbreaker = 0x4B9694;
constexpr uint32_t kFeDatabase = 0x48EDD4, kProfileCash = 0xB4, kCheatCash = 9999999;
constexpr uint32_t kCopManager = 0x494E40, kNoNewCops = 0x60;
constexpr uint32_t kUnlockAllThings = 0x493D00;

struct HeldByte {
    uint32_t held = 0;
    uint8_t before = 0;
    void Apply(bool on, uint32_t addr) {
        if (held && held != addr) held = 0;
        if (on && addr) {
            if (!held) {
                held = addr;
                before = kernel::G<uint8_t>(addr);
            }
            kernel::G<uint8_t>(addr) = 1;
        } else if (held) {
            kernel::G<uint8_t>(held) = before;
            held = 0;
        }
    }
};

static std::string MusicLatin1(const std::string& text) {
    std::string out;
    for (size_t i = 0; i < text.size(); i++) {
        uint8_t c = uint8_t(text[i]);
        if (c < 0x80) {
            out += char(c);
        } else if ((c & 0xE0) == 0xC0 && i + 1 < text.size()) {
            uint32_t cp = (c & 0x1Fu) << 6 | (uint8_t(text[++i]) & 0x3Fu);
            out += cp < 0x100 ? char(cp) : '?';
        } else {
            while (i + 1 < text.size() && (uint8_t(text[i + 1]) & 0xC0) == 0x80) i++;
            out += '?';
        }
    }
    return out;
}

static uint32_t MusicText(const std::string& text) {
    static std::unordered_map<std::string, uint32_t> copies;
    uint32_t& va = copies[text];
    if (!va) {
        std::string latin = MusicLatin1(text);
        va = mem::AllocSystem(uint32_t(latin.size() + 1));
        if (va) std::memcpy(kernel::HostPtr(va), latin.c_str(), latin.size() + 1);
    }
    return va;
}

static bool MusicMayBegin() {
    constexpr uint32_t kFEng = 0x48E94C, kFirstPackage = 0xE0, kPackageNext = 4, kPackageName = 0xC;
    static bool reached = false;
    if (reached) return true;
    if (Racing()) return reached = true;
    uint32_t fe = kernel::R32(kFEng);
    fe = fe && mem::IsCommitted(fe) ? kernel::R32(fe) : 0;
    uint32_t package = fe && mem::IsCommitted(fe + kFirstPackage) ? kernel::R32(fe + kFirstPackage) : 0;
    for (int n = 0; package && n < 64 && mem::IsCommitted(package + kPackageName); n++) {
        uint32_t name = kernel::R32(package + kPackageName);
        if (name && mem::IsCommitted(name + 7) && !strncasecmp(static_cast<const char*>(kernel::HostPtr(name)), "MainMenu", 8))
            return reached = true;
        package = kernel::R32(package + kPackageNext);
    }
    return false;
}

std::atomic<int> g_nis{0};

constexpr uint32_t kMovieTick = 0xC98356BA;
std::atomic<int64_t> g_movie_tick_ms{INT64_MIN / 2};
int64_t NowMs() {
    return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();
}
void NoteMovie(uint32_t message) {
    if (message == kMovieTick) g_movie_tick_ms = NowMs();
}
bool MoviePlaying() { return NowMs() - g_movie_tick_ms < 500; }

void MusicFrame(X86Context& ctx) {
    constexpr uint32_t kFeDatabase = 0x48EDD4, kProfile = 0x10, kEaTrax = 0x78, kMenuMusic = 0x54, kGameMusic = 0x58;
    constexpr uint32_t kInteractiveMusic = 0x74;
    constexpr uint32_t kShowSong = 0x1443C0;
    uint32_t db = kernel::R32(kFeDatabase);
    uint32_t profile = db && mem::IsCommitted(db + kProfile) ? kernel::R32(db + kProfile) : 0;
    if (!profile || !mem::IsCommitted(profile + kGameMusic)) return;
    static uint32_t restored_in = 0;
    if (!Opt().music_restored && restored_in != profile) {
        kernel::W32(profile + kEaTrax, 1);
        kernel::W32(profile + kInteractiveMusic, 1);
        restored_in = profile;
        if (Racing()) {
            Opt().music_restored = true;
            app::Save();
        }
    }
    static bool muted = false;
    static uint32_t saved_ea = 1, saved_interactive = 1;
    uint32_t flow = mem::IsCommitted(0x4936B8) ? kernel::R32(0x4936B8) : 3;
    bool loading = flow != 3 && flow != 6;
    if (flow == 3) g_nis = 0;
    if (ReplayActive() || loading || g_nis > 0 || MoviePlaying()) {
        if (!muted) {
            saved_ea = kernel::R32(profile + kEaTrax);
            saved_interactive = kernel::R32(profile + kInteractiveMusic);
            muted = true;
        }
        kernel::W32(profile + kEaTrax, 0);
        kernel::W32(profile + kInteractiveMusic, 0);
        if (music::HasSongs()) music::SetContext(MusicMayBegin(), Racing(), 0.0f);
        return;
    }
    if (muted) {
        kernel::W32(profile + kEaTrax, saved_ea);
        kernel::W32(profile + kInteractiveMusic, saved_interactive);
        muted = false;
    }
    if (!music::HasSongs()) return;
    static int source = -1;
    int now = music::Source();
    if (now == music::kMyMusic) {
        kernel::W32(profile + kEaTrax, 0);
    } else if (source == music::kMyMusic) {
        kernel::W32(profile + kEaTrax, 1);
    }
    source = now;
    uint32_t bits = kernel::R32(profile + (Racing() ? kGameMusic : kMenuMusic));
    float level;
    std::memcpy(&level, &bits, sizeof level);
    music::SetContext(MusicMayBegin(), Racing(), std::clamp(level, 0.0f, 1.0f));
    music::Song song;
    if (music::TakeStarted(&song)) {
        uint32_t saved[] = {ctx.eax, ctx.ecx, ctx.edx, ctx.ebx, ctx.esi, ctx.edi, ctx.ebp};
        kernel::CallGuest(ctx, kShowSong, {MusicText(song.title), MusicText(song.artist), MusicText(song.album)});
        ctx.eax = saved[0], ctx.ecx = saved[1], ctx.edx = saved[2], ctx.ebx = saved[3], ctx.esi = saved[4], ctx.edi = saved[5],
        ctx.ebp = saved[6];
    }
}

void ApplyCheats() {
    const Options& o = Opt();
    kernel::G<uint8_t>(kInfiniteNos) = o.cheat_nos ? 1 : 0;
    kernel::G<uint8_t>(kInfiniteSpeedbreaker) = o.cheat_speedbreaker ? 1 : 0;
    if (o.cheat_cash) {
        uint32_t db = kernel::R32(kFeDatabase);
        uint32_t profile = db && mem::IsCommitted(db + 0x10) ? kernel::R32(db + 0x10) : 0;
        if (profile && mem::IsCommitted(profile + kProfileCash)) kernel::W32(profile + kProfileCash, kCheatCash);
    }
    static HeldByte no_cops, unlock_all;
    uint32_t mgr = kernel::R32(kCopManager);
    no_cops.Apply(o.cheat_no_police, mgr && mem::IsCommitted(mgr + kNoNewCops) ? mgr + kNoNewCops : 0);
    unlock_all.Apply(o.cheat_unlock_all, kUnlockAllThings);
}

constexpr uint32_t kMsgTick = 0xC98356BA, kMsgPadAccept = 0x406415E3, kMsgPadStart = 0xB5AF2461;
constexpr int kSkipTick = 10;
std::unordered_map<uint32_t, int> g_movie_ticks;

void SkipMovie(X86Context& ctx, uint32_t allow_flag) {
    uint32_t message = kernel::R32(ctx.esp + 4);
    bool press = message == kMsgPadAccept || message == kMsgPadStart;
    if (!Opt().skip_movies || (!press && message != kMsgTick)) return;
    int& ticks = g_movie_ticks[ctx.ecx];
    if (press) {
        if (ticks > kSkipTick) kernel::W32(ctx.esp + 4, kMsgTick);
        else if (!allow_flag || kernel::G<uint8_t>(ctx.ecx + allow_flag)) ticks = kSkipTick + 1;
        return;
    }
    if (ticks > kSkipTick || ++ticks < kSkipTick) return;
    if (allow_flag && !kernel::G<uint8_t>(ctx.ecx + allow_flag)) {
        ticks = kSkipTick - 1;
        return;
    }
    ticks = kSkipTick + 1;
    kernel::W32(ctx.esp + 4, kMsgPadAccept);
}
void ForgetMovie(uint32_t screen) { g_movie_ticks.erase(screen); }

constexpr uint32_t kPlayers = 0x49550C, kPlayerCount = 0x495514, kFindInterface = 0x169A80, kIResetable = 0x19AFC0;
constexpr uint32_t kResetVehicle = 8;
void ResetCarFrame(X86Context& ctx) {
    if (!g_reset_car.exchange(false) || !Racing()) return;
    struct Restore {
        X86Context& destination;
        X86Context saved;
        ~Restore() { destination = saved; }
    } restore{ctx, ctx};
    uint32_t list = kernel::R32(kPlayers);
    uint32_t player = kernel::R32(kPlayerCount) && list ? kernel::R32(list) : 0;
    uint32_t object = player ? kernel::R32(player + 4) : 0;
    if (!object) return;
    ctx.ecx = object;
    uint32_t resetable = kernel::CallGuest(ctx, kFindInterface, {kIResetable});
    if (!resetable) return;
    ctx.ecx = resetable;
    uint32_t done = kernel::CallGuest(ctx, kernel::R32(kernel::R32(resetable) + kResetVehicle), {0}) & 0xFF;
    LOGI("reset car: %s", done ? "done" : "no place kept");
}

bool LanguageAvailable(const std::string& code) { return code == "int" || code == "ind"; }

constexpr uint32_t kGameFlowState = 0x4936B8;
constexpr uint32_t kGameFlowRacing = 6;
bool Racing() { return mem::IsCommitted(kGameFlowState) && kernel::R32(kGameFlowState) == kGameFlowRacing; }

}  // namespace game

X86_EXTERN_FUNC(__imp__sub_000CC530);
X86_FUNC(sub_000CC530) {
    if (game::Opt().rain_drops) return __imp__sub_000CC530(ctx, base);
    ctx.esp += 4;
}

X86_EXTERN_FUNC(__imp__sub_002527A0);
X86_FUNC(sub_002527A0) {
    if (game::Opt().rain) return __imp__sub_002527A0(ctx, base);
    ctx.esp += 4;
}

X86_EXTERN_FUNC(__imp__sub_000C1BD0);
X86_FUNC(sub_000C1BD0) {
    if (game::Opt().yellow_tint && !game::NightMode()) return __imp__sub_000C1BD0(ctx, base);
    uint32_t tint = ctx.ecx + 0xA0;
    float& r = kernel::G<float>(tint);
    float& g = kernel::G<float>(tint + 4);
    float& b = kernel::G<float>(tint + 8);
    float saved[] = {r, g, b};
    float luma = 0.299f * saved[0] + 0.587f * saved[1] + 0.114f * saved[2];
    switch (game::Opt().time_of_day) {
        case game::kTimeMorning:
            // a soft warm tint, still easy on the eyes
            r = 1.05f * luma;
            g = 1.00f * luma;
            b = 0.90f * luma;
            break;
        case game::kTimeAfternoon:
            // a golden hour, warmer and a little darker in the blue
            r = 1.18f * luma;
            g = 0.92f * luma;
            b = 0.68f * luma;
            break;
        case game::kTimeNight:
            // a cool, dark blue
            r = 0.15f * luma;
            g = 0.32f * luma;
            b = 0.68f * luma;
            break;
        case game::kTimeDay:
        default:
            // the original neutral tone
            r = g = b = luma;
            break;
    }
    __imp__sub_000C1BD0(ctx, base);
    r = saved[0], g = saved[1], b = saved[2];
}

X86_EXTERN_FUNC(__imp__sub_000CDA40);
X86_FUNC(sub_000CDA40) {
    if (!game::NightMode()) return __imp__sub_000CDA40(ctx, base);
    uint32_t obj = ctx.ecx;
    float& bloom = kernel::G<float>(obj + 0x170);
    float saved_bloom = bloom;
    bloom = std::min(bloom, 0.20f);
    __imp__sub_000CDA40(ctx, base);
    bloom = saved_bloom;
}

X86_EXTERN_FUNC(__imp__sub_000D5AB0);
X86_FUNC(sub_000D5AB0) {
    if (game::NightMode()) {
        ctx.esp += 4;
        return;
    }
    __imp__sub_000D5AB0(ctx, base);
}

X86_EXTERN_FUNC(__imp__sub_000D5CA0);
X86_FUNC(sub_000D5CA0) {
    if (game::NightMode()) {
        ctx.esp += 4;
        return;
    }
    __imp__sub_000D5CA0(ctx, base);
}

X86_EXTERN_FUNC(__imp__sub_0024AB90);
X86_FUNC(sub_0024AB90) {
    uint32_t ptr = kernel::R32(ctx.esp + 4);
    if (ptr && mem::IsCommitted(ptr)) {
        if (game::NightMode()) {
            kernel::W32(ptr, kernel::R32(ptr) | 7);
        } else if (game::g_headlights_clear_frames > 0) {
            kernel::W32(ptr, kernel::R32(ptr) & ~7u);
        }
    }
    __imp__sub_0024AB90(ctx, base);
}

X86_EXTERN_FUNC(__imp__sub_0024ABE0);
X86_FUNC(sub_0024ABE0) {
    if (game::NightMode()) {
        kernel::W32(ctx.esp + 20, 1);
    }
    __imp__sub_0024ABE0(ctx, base);
}

X86_EXTERN_FUNC(__imp__sub_00244B70);
X86_FUNC(sub_00244B70) {
    constexpr uint32_t kSkyColour = 0x42FE00, kSkyLoopReturn = 0x0D7792;
    if (!game::NightMode() || kernel::R32(ctx.esp) == kSkyLoopReturn) return __imp__sub_00244B70(ctx, base);
    static const float kNight[3] = {0.10f, 0.15f, 0.32f};
    float saved[3];
    for (int i = 0; i < 3; i++) {
        float& c = kernel::G<float>(kSkyColour + i * 4);
        saved[i] = c;
        c *= kNight[i];
    }
    __imp__sub_00244B70(ctx, base);
    for (int i = 0; i < 3; i++) kernel::G<float>(kSkyColour + i * 4) = saved[i];
}

X86_EXTERN_FUNC(__imp__sub_000D1440);
X86_FUNC(sub_000D1440) {
    if (game::Opt().motion_blur && !game::CinematicActive() && !game::ReplayActive())
        return __imp__sub_000D1440(ctx, base);
    ctx.eax &= ~0xFFu;
    ctx.esp += 4;
}

// FIX (NDK 28 / Clang 18): sub_002BC401 and sub_002BC40C are wrapper functions (dispatch-chain entry points),
// distinct from their __imp__ counterparts declared below. Older toolchains let the wrapper calls through
// undeclared; current Clang requires the wrapper names to be declared too.
X86_EXTERN_FUNC(__imp__sub_00087A40);
X86_EXTERN_FUNC(__imp__sub_002BC401);
X86_EXTERN_FUNC(__imp__sub_002BC40C);
X86_EXTERN_FUNC(sub_002BC401);
X86_EXTERN_FUNC(sub_002BC40C);
X86_FUNC(sub_00087A40) {
    constexpr uint32_t kStrm = 0x4D525453;
    uint32_t handle = kernel::R32(ctx.esp + 4);
    uint32_t system = handle ? kernel::R32(handle) : 0;
    if (!system || !mem::IsCommitted(system) || kernel::R32(system) != kStrm) return __imp__sub_00087A40(ctx, base);
    ctx.ecx = system + 4;
    X86_PUSH32(0);
    sub_002BC401(ctx, base);
    __imp__sub_00087A40(ctx, base);
    uint32_t result = ctx.eax;
    ctx.ecx = system + 4;
    X86_PUSH32(0);
    sub_002BC40C(ctx, base);
    ctx.eax = result;
}

namespace {
constexpr uint32_t kITransmission = 0x11FB0, kDragTransmissionVtable = 0x3E3560, kTransmissionShift = 0xC;
constexpr uint32_t kEngineFromTransmission = 0x50;
constexpr uint32_t kEngineRpm = 0x184, kEngineInfo = 0x150, kInfoRedline = 0x58, kEngineGear = 0x88;
constexpr uint32_t kEngineClutch = 0x19C;
constexpr uint32_t kFirstGear = 2;
constexpr float kShiftAt = 0.97f;
constexpr uint64_t kShiftRetryMs = 600;

uint64_t SteadyMs() {
    using namespace std::chrono;
    return uint64_t(duration_cast<milliseconds>(steady_clock::now().time_since_epoch()).count());
}

float GuestF32(uint32_t va) {
    uint32_t v = kernel::R32(va);
    float f;
    std::memcpy(&f, &v, sizeof(f));
    return f;
}
}  // namespace

namespace game {
void DragShiftFrame(X86Context& ctx) {
    if (!Opt().cheat_drag_shift || !Racing()) return;
    uint32_t list = kernel::R32(kPlayers);
    uint32_t player = kernel::R32(kPlayerCount) && list ? kernel::R32(list) : 0;
    uint32_t object = player ? kernel::R32(player + 4) : 0;
    if (!object) return;
    struct Restore {
        X86Context& destination;
        X86Context saved;
        ~Restore() { destination = saved; }
    } restore{ctx, ctx};
    ctx.ecx = object;
    uint32_t transmission = kernel::CallGuest(ctx, kFindInterface, {kITransmission});
    if (!transmission || kernel::R32(transmission) != kDragTransmissionVtable) return;
    uint32_t engine = transmission - kEngineFromTransmission;
    uint32_t info = kernel::R32(engine + kEngineInfo);
    if (!mem::IsCommitted(info + kInfoRedline)) return;
    static uint32_t last_gear = ~0u;
    static uint64_t sent_ms = 0;
    uint32_t gear = kernel::R32(engine + kEngineGear);
    float rpm = GuestF32(engine + kEngineRpm), redline = GuestF32(info + kInfoRedline);
    uint64_t now = SteadyMs();
    if (gear != last_gear) {
        LOGI("drag shift: gear %u, rpm %.0f, redline %.0f", gear, rpm, redline);
        last_gear = gear;
        sent_ms = 0;
    }
    if (gear < kFirstGear || kernel::R32(engine + kEngineClutch) != 0) return;
    if (!(redline > 0) || rpm < redline * kShiftAt) return;
    if (sent_ms && now - sent_ms < kShiftRetryMs) return;
    sent_ms = now;
    ctx.ecx = transmission;
    uint32_t done = kernel::CallGuest(ctx, kernel::R32(kDragTransmissionVtable + kTransmissionShift), {gear + 1}) & 0xFF;
    LOGI("drag shift: up from gear %u, rpm %.0f: %s", gear, rpm, done ? "shifted" : "refused");
}
}  // namespace game

namespace {
constexpr uint32_t kTrafficCars = 0x94, kNodeCar = 8;
constexpr uint32_t kCarIsActive = 0x88, kCarAi = 0xAC, kAiLeave = 0x8C;
uint32_t VCall(X86Context& ctx, uint32_t self, uint32_t slot) {
    ctx.ecx = self;
    return kernel::CallGuest(ctx, kernel::R32(kernel::R32(self) + slot), {});
}
}  // namespace

X86_EXTERN_FUNC(__imp__sub_00022CC0);
X86_FUNC(sub_00022CC0) {
    __imp__sub_00022CC0(ctx, base);
    if (game::Opt().cheat_no_traffic) X87_ST(0) = 0.0;
}

X86_EXTERN_FUNC(__imp__sub_00035A50);
X86_FUNC(sub_00035A50) {
    uint32_t manager = ctx.ecx;
    __imp__sub_00035A50(ctx, base);
    if (!game::Opt().cheat_no_traffic) return;
    struct Restore {
        X86Context& destination;
        X86Context saved;
        ~Restore() { destination = saved; }
    } restore{ctx, ctx};
    uint32_t head = kernel::R32(manager + kTrafficCars);
    if (!head) return;
    int released = 0;
    for (uint32_t node = kernel::R32(head); node && node != head; node = kernel::R32(node)) {
        uint32_t car = kernel::R32(node + kNodeCar);
        if (!car || !(VCall(ctx, car, kCarIsActive) & 0xFF)) continue;
        if (uint32_t ai = VCall(ctx, car, kCarAi)) {
            VCall(ctx, ai, kAiLeave);
            released++;
        }
    }
    if (released) LOGI("no traffic: %d cars let go", released);
}

namespace {
constexpr uint32_t kSpawnState = 0x4C, kSpawnAttributes = 4, kAttribute = 0x490A0, kModelKey = 0xF833C06F;
constexpr uint8_t kSpawnMaking = 1;
}  // namespace

X86_EXTERN_FUNC(__imp__sub_00181120);
X86_FUNC(sub_00181120) {
    uint32_t spawner = ctx.ecx;
    uint8_t state = *static_cast<const uint8_t*>(kernel::HostPtr(spawner + kSpawnState));
    if (game::Opt().cheat_no_traffic && state == kSpawnMaking) {
        X86Context saved = ctx;
        ctx.ecx = spawner + kSpawnAttributes;
        uint32_t model = kernel::CallGuest(ctx, kAttribute, {kModelKey, 0});
        ctx = saved;
        const char* name = model && mem::IsCommitted(model) && kernel::R32(model)
                               ? static_cast<const char*>(kernel::HostPtr(kernel::R32(model)))
                               : nullptr;
        if (name && std::strcmp(name, "copmidsize") != 0) {
            ctx.eax = 0;
            ctx.esp += 4;
            return;
        }
    }
    __imp__sub_00181120(ctx, base);
}

X86_EXTERN_FUNC(__imp__sub_00116230);
X86_FUNC(sub_00116230) {
    game::NoteMovie(kernel::R32(ctx.esp + 4));
    game::SkipMovie(ctx, 0);
    __imp__sub_00116230(ctx, base);
}
X86_EXTERN_FUNC(__imp__sub_00131A30);
X86_FUNC(sub_00131A30) {
    game::NoteMovie(kernel::R32(ctx.esp + 4));
    game::SkipMovie(ctx, 0);
    __imp__sub_00131A30(ctx, base);
}
X86_EXTERN_FUNC(__imp__sub_001311B0);
X86_FUNC(sub_001311B0) {
    game::NoteMovie(kernel::R32(ctx.esp + 4));
    game::SkipMovie(ctx, 0x2C);
    __imp__sub_001311B0(ctx, base);
}
X86_EXTERN_FUNC(__imp__sub_0010D6D0);
X86_FUNC(sub_0010D6D0) {
    game::ForgetMovie(ctx.ecx);
    __imp__sub_0010D6D0(ctx, base);
}
X86_EXTERN_FUNC(__imp__sub_0013F6D0);
X86_FUNC(sub_0013F6D0) {
    game::ForgetMovie(ctx.ecx);
    __imp__sub_0013F6D0(ctx, base);
}
X86_EXTERN_FUNC(__imp__sub_0013EEE0);
X86_FUNC(sub_0013EEE0) {
    game::ForgetMovie(ctx.ecx);
    __imp__sub_0013EEE0(ctx, base);
}
X86_EXTERN_FUNC(__imp__sub_002C020F);
X86_FUNC(sub_002C020F) {
    if (!game::MoviePlaying()) LOGR("music: a movie plays (its stream read), the music held");
    game::NoteMovie(game::kMovieTick);
    __imp__sub_002C020F(ctx, base);
}

namespace app {
uint32_t ConsoleLanguage() { return 1; }
}  // namespace app

X86_EXTERN_FUNC(__imp__sub_00214360);
X86_FUNC(sub_00214360) {
    game::g_nis++;
    LOGR("music: cutscene began (flow %u, %d playing)", kernel::R32(0x4936B8), game::g_nis.load());
    __imp__sub_00214360(ctx, base);
}

X86_EXTERN_FUNC(__imp__sub_00215CC0);
X86_FUNC(sub_00215CC0) {
    __imp__sub_00215CC0(ctx, base);
    if (game::g_nis > 0) game::g_nis--;
    LOGR("music: cutscene ended (flow %u, %d playing)", kernel::R32(0x4936B8), game::g_nis.load());
}
