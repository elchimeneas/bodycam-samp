#include "config.h"
#include <algorithm>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <sstream>

namespace bodycam {
static std::string Trim(std::string s) {
    const auto first = s.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) return {};
    return s.substr(first, s.find_last_not_of(" \t\r\n") - first + 1);
}
static int Number(const std::string& s, int fallback, int lo, int hi) {
    try {
        size_t end = 0;
        const long n = std::stol(s, &end, 0);
        return end == s.size() ? static_cast<int>(std::clamp(n, long(lo), long(hi))) : fallback;
    } catch (...) { return fallback; }
}
static std::wstring Label(const std::string& s, const std::wstring& fallback, size_t limit) {
    if (s.empty()) return L"";
    const int len = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, s.data(), int(s.size()), nullptr, 0);
    if (!len) return fallback;
    std::wstring out(len, L' ');
    MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, s.data(), int(s.size()), out.data(), len);
    for (auto& c : out) if (c < 32 || c == 127) c = L' ';
    if (out.size() > limit) out.resize(limit);
    if (!out.empty() && out.back() >= 0xD800 && out.back() <= 0xDBFF) out.pop_back();
    return out;
}
static double Real(const std::string& s,double fallback,double lo,double hi) {
    try {size_t end=0;const double d=std::stod(s,&end);return end==s.size() && std::isfinite(d)?std::clamp(d,lo,hi):fallback;}
    catch (...) {return fallback;}
}
Config ParseConfig(std::string_view text) {
    Config c;
    if (text.substr(0, 3) == "\xEF\xBB\xBF") text.remove_prefix(3);
    std::istringstream in{std::string(text)};
    std::string line;
    while (std::getline(in, line)) {
        line = Trim(line);
        if (line.empty() || line[0] == ';' || line[0] == '#' || line[0] == '[') continue;
        const auto sep = line.find('=');
        if (sep == std::string::npos) continue;
        const auto k = Trim(line.substr(0, sep)), v = Trim(line.substr(sep + 1));
        if (k == "Enabled") c.enabled = Number(v, 1, 0, 1) != 0;
        else if (k == "UseUTC") c.utc = Number(v, 0, 0, 1) != 0;
        else if (k == "ShowOfficer") c.showOfficer = Number(v, 1, 0, 1) != 0;
        else if (k == "ShowLogo") c.showLogo = Number(v, 1, 0, 1) != 0;
        else if (k == "Officer") c.officer = Label(v, c.officer, 48);
        else if (k == "Badge") c.badge = Label(v, c.badge, 16);
        else if (k == "CameraLabel") c.cameraLabel = Label(v, c.cameraLabel, 48);
        else if (k == "CameraId") c.cameraId = Label(v, c.cameraId, 24);
        else if (k == "FontSize") c.fontSize = Number(v, 20, 12, 36);
        else if (k == "MarginRight") c.marginRight = Number(v, 260, 0, 1920);
        else if (k == "MarginTop") c.marginTop = Number(v, 24, 0, 1080);
        else if (k == "Opacity") c.opacity = Number(v, 235, 40, 255);
        else if (k == "ToggleKey") c.toggleKey = Number(v, 'B', 1, 254);
        else if (k == "ReloadKey") c.reloadKey = Number(v, 'R', 1, 254);
        else if (k == "Modifiers") c.modifiers = Number(v, 3, 0, 7);
        else if (k == "CameraEnabled") c.cameraEnabled = Number(v,1,0,1)!=0;
        else if (k == "CameraInVehicles") c.cameraVehicles = Number(v,1,0,1)!=0;
        else if (k == "CameraFOV") c.cameraFov = Real(v,100,60,120);
        else if (k == "CameraNearClip") c.cameraNear = Real(v,Config{}.cameraNear,0.04,0.25);
        else if (k == "CameraYawLimit") c.cameraYawLimit = Real(v,Config{}.cameraYawLimit,10,90);
        else if (k == "ChestForward") c.chestForward = Real(v,Config{}.chestForward,0.05,0.35);
        else if (k == "ChestSide") c.chestSide = Real(v,Config{}.chestSide,-0.20,0.20);
        else if (k == "ChestHeight") c.chestHeight = Real(v,Config{}.chestHeight,-0.30,0.30);
        else if (k == "AimForwardOffset") c.aimForwardOffset = Real(v,Config{}.aimForwardOffset,-0.15,0.15);
        else if (k == "AimHeightOffset") c.aimHeightOffset = Real(v,Config{}.aimHeightOffset,-0.20,0.20);
        else if (k == "RecordingEffect") c.recordingEffect = Number(v,1,0,1)!=0;
        else if (k == "EffectIntensity") c.effectIntensity = Real(v,0.25,0,1);
        else if (k == "Scale") {
            try {
                size_t end = 0; const double d = std::stod(v, &end);
                if (end == v.size() && std::isfinite(d)) c.scale = std::clamp(d, 0.5, 2.0);
            } catch (...) {}
        }
    }
    return c;
}
Config LoadConfig(const std::wstring& path) {
    std::ifstream in(std::filesystem::path(path), std::ios::binary);
    if (!in) return {};
    std::string data(16384, '\0');
    in.read(data.data(), std::streamsize(data.size()));
    data.resize(static_cast<size_t>(in.gcount()));
    return ParseConfig(data);
}
std::wstring Timestamp(const SYSTEMTIME& t, bool utc, int offset) {
    wchar_t b[80]{};
    if (utc) swprintf_s(b, L"%04u-%02u-%02u %02u:%02u:%02u Z", t.wYear,t.wMonth,t.wDay,t.wHour,t.wMinute,t.wSecond);
    else swprintf_s(b, L"%04u-%02u-%02u %02u:%02u:%02u %c%02d%02d", t.wYear,t.wMonth,t.wDay,t.wHour,t.wMinute,t.wSecond,
        offset < 0 ? L'-' : L'+', std::abs(offset) / 60, std::abs(offset) % 60);
    return b;
}
std::wstring CurrentTimestamp(bool utc) {
    SYSTEMTIME t{};
    int offset = 0;
    if (utc) GetSystemTime(&t);
    else {
        GetLocalTime(&t);
        TIME_ZONE_INFORMATION tz{};
        const DWORD kind = GetTimeZoneInformation(&tz);
        if (kind != TIME_ZONE_ID_INVALID) offset = -int(tz.Bias + (kind == TIME_ZONE_ID_DAYLIGHT ? tz.DaylightBias : tz.StandardBias));
    }
    return Timestamp(t, utc, offset);
}
std::wstring CameraLine(const Config& c) {
    if (c.cameraLabel.empty()) return c.cameraId;
    if (c.cameraId.empty()) return c.cameraLabel;
    return c.cameraLabel + L"  " + c.cameraId;
}
Layout MakeLayout(const Config& c, int w, int h) {
    Layout l{};
    const double res = std::clamp(h / 1080.0, 0.4, 4.0);
    l.scale = std::min({res * c.scale, std::max(0.2, (w - 8.0) / 720.0), 3.0});
    l.font = std::clamp(int(std::lround(c.fontSize * l.scale)), 8, 100);
    l.line = l.font + std::max(3, l.font / 5);
    l.pad = std::max(3, l.font / 4);
    l.logo = c.showLogo ? l.line * 2 : 0;
    size_t characters=std::max(size_t(25),CameraLine(c).size());
    if (c.showOfficer) characters=std::max(characters,c.officer.size()+(c.badge.empty()?0:8+c.badge.size()));
    const double content=characters*l.font*0.65+l.logo+l.pad*3;
    l.width = std::min(w, int(std::ceil(std::min(720*l.scale,content))));
    l.height = l.line * (c.showOfficer ? 3 : 2) + l.pad * 2;
    l.height = std::min(l.height, h);
    l.x = std::clamp(w - l.width - int(c.marginRight * res), 0, std::max(0, w - l.width));
    l.y = std::clamp(int(c.marginTop * res), 0, std::max(0, h - l.height));
    return l;
}
}
