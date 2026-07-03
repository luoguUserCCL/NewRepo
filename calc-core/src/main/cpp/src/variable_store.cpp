#include <calc/variable_store.h>

namespace calc {

VariableStore::VariableStore() {}

void VariableStore::set(const std::string& name, const BigDecimal& value) {
    variables_[name] = value;
}

const BigDecimal* VariableStore::get(const std::string& name) const {
    auto it = variables_.find(name);
    return it != variables_.end() ? &it->second : nullptr;
}

bool VariableStore::has(const std::string& name) const {
    return variables_.count(name) > 0;
}

bool VariableStore::remove(const std::string& name) {
    return variables_.erase(name) > 0;
}

void VariableStore::clear() {
    variables_.clear();
}

std::vector<std::string> VariableStore::listVariables() const {
    std::vector<std::string> names;
    names.reserve(variables_.size());
    for (const auto& [name, _] : variables_) {
        names.push_back(name);
    }
    return names;
}

size_t VariableStore::size() const {
    return variables_.size();
}

void VariableStore::registerConstants() {
    variables_["pi"] = BigDecimal::pi();
    variables_["e"] = BigDecimal::e();
    variables_["phi"] = BigDecimal("1.61803398874989484820458683436563811772030917980576286213544862270526046281890244970720720418939113748475");
    // Golden ratio φ = (1+√5)/2
}

} // namespace calc
