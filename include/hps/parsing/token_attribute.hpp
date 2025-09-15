#pragma once
#include <string>

namespace hps {
struct TokenAttribute {
    std::string m_name;
    std::string m_value;
    bool        m_has_value = true;  // 区分 <div checked> 和 <div checked="true">

    TokenAttribute() = default;

    explicit TokenAttribute(const std::string_view n, const std::string_view v = {}, const bool hv = true) : m_name(n), m_value(v), m_has_value(hv) {}
};
}  // namespace hps