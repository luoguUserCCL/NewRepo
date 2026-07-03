#include "i18n.h"

namespace calc {

I18n::Language I18n::currentLang_ = Language::EN;
std::unordered_map<std::string, std::string> I18n::enDict_;
std::unordered_map<std::string, std::string> I18n::zhDict_;
bool I18n::dictsInitialized_ = false;

void I18n::initDicts() {
    if (dictsInitialized_) return;
    dictsInitialized_ = true;

    // ========== English ==========
    enDict_ = {
        {"app.title",           "Scientific Calculator"},
        {"menu.file",           "File"},
        {"menu.edit",           "Edit"},
        {"menu.settings",       "Settings"},
        {"menu.help",           "Help"},
        {"input.placeholder",   "Enter expression..."},
        {"input.evaluate",      "Evaluate"},
        {"input.clear",         "Clear"},
        {"mode.scientific",     "Scientific"},
        {"mode.fixed",          "Fixed Point"},
        {"mode.significant",    "Significant Digits"},
        {"mode.floating",       "Floating Point"},
        {"base.binary",         "Binary (2)"},
        {"base.octal",          "Octal (8)"},
        {"base.decimal",        "Decimal (10)"},
        {"base.hex",            "Hexadecimal (16)"},
        {"base.prefix",         "Show base prefix"},
        {"precision",           "Precision"},
        {"digits",              "Digits"},
        {"func.define",         "Define Function"},
        {"var.define",          "Define Variable"},
        {"func.name",           "Function Name"},
        {"func.params",         "Parameters"},
        {"func.body",           "Function Body"},
        {"var.name",            "Variable Name"},
        {"var.value",           "Value"},
        {"history",             "History"},
        {"history.empty",       "No history yet"},
        {"result",              "Result"},
        {"error",               "Error"},
        {"settings.title",      "Settings"},
        {"settings.language",   "Language"},
        {"settings.format",     "Number Format"},
        {"settings.base",       "Number Base"},
        {"lang.english",        "English"},
        {"lang.chinese",        "Chinese"},
        {"tab.calculator",      "Calculator"},
        {"tab.functions",       "Functions"},
        {"tab.variables",       "Variables"},
        {"btn.add",             "Add"},
        {"btn.remove",          "Remove"},
        {"btn.reset",           "Reset"},
    };

    // ========== Chinese ==========
    zhDict_ = {
        {"app.title",           "科学计算器"},
        {"menu.file",           "文件"},
        {"menu.edit",           "编辑"},
        {"menu.settings",       "设置"},
        {"menu.help",           "帮助"},
        {"input.placeholder",   "输入表达式..."},
        {"input.evaluate",      "计算"},
        {"input.clear",         "清空"},
        {"mode.scientific",     "科学计数"},
        {"mode.fixed",          "定点小数"},
        {"mode.significant",    "有效位数"},
        {"mode.floating",       "浮点数"},
        {"base.binary",         "二进制 (2)"},
        {"base.octal",          "八进制 (8)"},
        {"base.decimal",        "十进制 (10)"},
        {"base.hex",            "十六进制 (16)"},
        {"base.prefix",         "显示进制前缀"},
        {"precision",           "精度"},
        {"digits",              "位数"},
        {"func.define",         "定义函数"},
        {"var.define",          "定义变量"},
        {"func.name",           "函数名"},
        {"func.params",         "参数"},
        {"func.body",           "函数体"},
        {"var.name",            "变量名"},
        {"var.value",           "值"},
        {"history",             "历史记录"},
        {"history.empty",       "暂无历史记录"},
        {"result",              "结果"},
        {"error",               "错误"},
        {"settings.title",      "设置"},
        {"settings.language",   "语言"},
        {"settings.format",     "数字格式"},
        {"settings.base",       "进制"},
        {"lang.english",        "英文"},
        {"lang.chinese",        "中文"},
        {"tab.calculator",      "计算器"},
        {"tab.functions",       "函数"},
        {"tab.variables",       "变量"},
        {"btn.add",             "添加"},
        {"btn.remove",          "删除"},
        {"btn.reset",           "重置"},
    };
}

const std::string& I18n::get(const std::string& key) {
    initDicts();
    static const std::string empty;
    const auto& dict = (currentLang_ == Language::ZH_CN) ? zhDict_ : enDict_;
    auto it = dict.find(key);
    return (it != dict.end()) ? it->second : empty;
}

void I18n::setLanguage(Language lang) {
    currentLang_ = lang;
}

I18n::Language I18n::getLanguage() {
    return currentLang_;
}

bool I18n::isChinese() {
    return currentLang_ == Language::ZH_CN;
}

} // namespace calc
