#ifndef CALC_VARIABLE_STORE_H
#define CALC_VARIABLE_STORE_H

#include <string>
#include <unordered_map>
#include <vector>
#include <calc/big_decimal.h>

namespace calc {

/**
 * Variable store for user-defined variables.
 * Supports definition, reassignment, deletion, and lookup.
 * Variables are defined with the := operator (e.g., x := 42).
 */
class VariableStore {
public:
    VariableStore();

    /// Define or update a variable
    void set(const std::string& name, const BigDecimal& value);

    /// Get a variable's value. Returns nullptr if not defined.
    const BigDecimal* get(const std::string& name) const;

    /// Check if a variable exists
    bool has(const std::string& name) const;

    /// Remove a variable
    bool remove(const std::string& name);

    /// Clear all user-defined variables
    void clear();

    /// List all variable names
    std::vector<std::string> listVariables() const;

    /// Get variable count
    size_t size() const;

    /// Pre-populate common mathematical constants
    void registerConstants();

private:
    std::unordered_map<std::string, BigDecimal> variables_;
};

} // namespace calc

#endif // CALC_VARIABLE_STORE_H
