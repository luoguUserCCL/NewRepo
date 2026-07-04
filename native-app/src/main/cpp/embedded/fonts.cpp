#include "fonts.h"
#include <fstream>
#include <vector>

#ifdef _WIN32
#include <windows.h>
#include <shlobj.h>
#else
#include <sys/stat.h>
#endif

namespace calc { namespace embedded {

/**
 * Check if a file exists and is readable.
 */
static bool fileExists(const std::string& path) {
#ifdef _WIN32
    DWORD attrs = GetFileAttributesA(path.c_str());
    return (attrs != INVALID_FILE_ATTRIBUTES && !(attrs & FILE_ATTRIBUTE_DIRECTORY));
#else
    struct stat st;
    return (stat(path.c_str(), &st) == 0 && S_ISREG(st.st_mode));
#endif
}

/**
 * Search for a font file by checking multiple candidate paths.
 */
static SystemFontResult searchPaths(const std::vector<std::string>& candidates) {
    for (const auto& path : candidates) {
        if (fileExists(path)) {
            return {true, path};
        }
    }
    return {false, ""};
}

SystemFontResult findNotoSansSC() {
    // ===== Linux paths =====
    std::vector<std::string> linuxPaths = {
        "/usr/share/fonts/truetype/chinese/NotoSansSC-Regular.otf",
        "/usr/share/fonts/truetype/chinese/NotoSansSC-Regular.ttf",
        "/usr/share/fonts/truetype/chinese/NotoSansSC[wght].ttf",
        "/usr/share/fonts/opentype/noto/NotoSansSC-Regular.otf",
        "/usr/share/fonts/noto-cjk/NotoSansSC-Regular.otf",
        "/usr/local/share/fonts/NotoSansSC-Regular.otf",
        std::string(getenv("HOME") ? getenv("HOME") : "") + "/.local/share/fonts/NotoSansSC-Regular.otf",
        std::string(getenv("HOME") ? getenv("HOME") : "") + "/.fonts/NotoSansSC-Regular.otf",
    };

    // ===== macOS paths =====
    std::vector<std::string> macPaths = {
        "/System/Library/Fonts/Supplemental/NotoSansSC-Regular.otf",
        "/Library/Fonts/NotoSansSC-Regular.otf",
        std::string(getenv("HOME") ? getenv("HOME") : "") + "/Library/Fonts/NotoSansSC-Regular.otf",
    };

    // ===== Windows paths =====
    std::vector<std::string> winPaths = {
        "C:\\Windows\\Fonts\\NotoSansSC-Regular.otf",
        "C:\\Windows\\Fonts\\NotoSansSC-Regular.ttf",
    };

    // Combine all paths based on platform
    std::vector<std::string> allPaths;
#ifdef __linux__
    allPaths = linuxPaths;
#elif __APPLE__
    allPaths = macPaths;
#elif _WIN32
    allPaths = winPaths;
#else
    allPaths = linuxPaths;
#endif

    // Also try all paths (some might work across platforms)
    for (const auto& p : macPaths) allPaths.push_back(p);
    for (const auto& p : linuxPaths) allPaths.push_back(p);
    for (const auto& p : winPaths) allPaths.push_back(p);

    return searchPaths(allPaths);
}

SystemFontResult findCJKFont() {
    // Common CJK fonts that can serve as fallback
    std::vector<std::string> candidates = {
        // Noto Sans SC (preferred)
        "/usr/share/fonts/truetype/chinese/NotoSansSC-Regular.otf",
        "/usr/share/fonts/truetype/chinese/NotoSansSC[wght].ttf",
        "/usr/share/fonts/opentype/noto/NotoSansSC-Regular.otf",
        // Noto Sans CJK SC
        "/usr/share/fonts/opentype/noto/NotoSansCJKsc-Regular.otf",
        "/usr/share/fonts/truetype/noto/NotoSansCJKsc-Regular.ttc",
        // WenQuanYi (common on Linux)
        "/usr/share/fonts/truetype/wqy/wqy-microhei.ttc",
        "/usr/share/fonts/truetype/wqy/wqy-zenhei.ttc",
        // Source Han Sans
        "/usr/share/fonts/opentype/noto/SourceHanSansSC-Regular.otf",
        // macOS
        "/System/Library/Fonts/PingFang.ttc",
        "/System/Library/Fonts/Supplemental/Songti.ttc",
        "/Library/Fonts/Songti.ttc",
        // Windows
        "C:\\Windows\\Fonts\\msyh.ttc",  // Microsoft YaHei
        "C:\\Windows\\Fonts\\simhei.ttf", // SimHei
        "C:\\Windows\\Fonts\\simsun.ttc", // SimSun
    };

    return searchPaths(candidates);
}

}} // namespace calc::embedded
