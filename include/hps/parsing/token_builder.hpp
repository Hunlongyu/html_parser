#pragma once
#include "token_attribute.hpp"

#include <vector>

namespace hps {
struct TokenBuilder {
    std::string                 tag_name;
    std::string                 attr_name;
    std::string                 attr_value;
    bool                        is_void_element = false;
    bool                        is_self_closing = false;
    std::vector<TokenAttribute> attrs;

    void add_attr(const TokenAttribute& attr) {
        attrs.push_back(attr);
    }

    void add_attr(std::string_view name, std::string_view value, bool has_value = true) {
        attrs.emplace_back(name, value, has_value);
    }

    void reset() {
        tag_name.clear();
        attr_name.clear();
        attr_value.clear();
        is_void_element = false;
        is_self_closing = false;
        attrs.clear();
    }

    void finish_current_attribute() {
        if (!attr_name.empty()) {
            add_attr(attr_name, attr_value);
            attr_name.clear();
            attr_value.clear();
        }
    }
};
}  // namespace hps