#pragma once
#include <windows.h>
#include <string>
#include <string_view>

namespace bodycam {
struct Config {
    bool enabled = true;
    bool utc = false;
    bool showOfficer = true;
    bool showLogo = true;
    double scale = 1.0;
    int fontSize = 20;
    int marginRight = 260;
    int marginTop = 24;
    int opacity = 235;
    int toggleKey = 'B';
    int reloadKey = 'R';
    int modifiers = 3; // Ctrl=1, Shift=2, Alt=4
    std::wstring officer = L"AGENTE";
    std::wstring badge;
    std::wstring cameraId = L"CAM-0001";
    bool cameraEnabled = true;
    bool cameraVehicles = true;
    double cameraFov = 100.0;
    double cameraNear = 0.04;
    double chestForward = 0.20;
    double chestSide = 0.01;
    double chestHeight = 0.16;
    double aimForwardOffset = -0.06;
    double aimHeightOffset = -0.10;
    bool recordingEffect = true;
    double effectIntensity = 0.25;
};
Config ParseConfig(std::string_view text);
Config LoadConfig(const std::wstring& path);
std::wstring Timestamp(const SYSTEMTIME& time, bool utc, int offsetMinutes);
std::wstring CurrentTimestamp(bool utc);
struct Layout { int width, height, font, line, logo, pad, x, y; double scale; };
Layout MakeLayout(const Config& config, int width, int height);
}
