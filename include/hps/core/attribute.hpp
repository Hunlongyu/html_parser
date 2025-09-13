#pragma once
#include "hps/parsing/token.hpp"

#include <format>
#include <string>
#include <string_view>

namespace hps {
class Attribute {
  public:
    constexpr Attribute() noexcept = default;

    explicit Attribute(const std::string_view name, const std::string_view value = {}, const bool hv = true) noexcept : m_name(name), m_value(value), m_has_value(hv) {}

    explicit Attribute(const TokenAttribute& attr) : m_name(attr.m_name), m_value(attr.m_value), m_has_value(attr.m_has_value) {}

    [[nodiscard]] const std::string& name() const noexcept {
        return m_name;
    }

    [[nodiscard]] const std::string& value() const noexcept {
        return m_value;
    }

    [[nodiscard]] bool has_value() const noexcept {
        return m_has_value;
    }

    [[nodiscard]] std::string to_string() const {
        if (!m_has_value) {
            return m_name;
        }
        return std::format("{}=\"{}\"", m_name, m_value);
    }

    void set_name(const std::string& name) noexcept {
        m_name = name;
    }

    void set_name(const std::string_view name) noexcept {
        m_name = name;
    }

    void set_value(const std::string& value, const bool has_value = true) noexcept {
        m_value     = value;
        m_has_value = has_value;
    }

    void set_value(const std::string_view value, const bool has_value = true) noexcept {
        m_value     = value;
        m_has_value = has_value;
    }

    auto operator<=>(const Attribute& other) const = default;

  private:
    std::string m_name;
    std::string m_value;
    bool        m_has_value = false;
};
}  // namespace hps