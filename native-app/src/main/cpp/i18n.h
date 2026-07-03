#ifndef CALC_I18N_H
#define CALC_I18N_H

#include <string>
#include <unordered_map>

namespace calc {

/**
 * Internationalization system — supports Chinese and English.
 * Simple key-value lookup with language switching.
 */
class I18n {
public:
    enum class Language {
        EN,
        ZH_CN
    };

    /// Get translation for a key in the current language
    static const std::string& get(const std::string& key);

    /// Set the current language
    static void setLanguage(Language lang);

    /// Get the current language
    static Language getLanguage();

    /// Check if current language is Chinese
    static bool isChinese();

private:
    static Language currentLang_;
    static std::unordered_map<std::string, std::string> enDict_;
    static std::unordered_map<std::string, std::string> zhDict_;
    static bool dictsInitialized_;

    static void initDicts();
};

} // namespace calc

#endif // CALC_I18N_H
