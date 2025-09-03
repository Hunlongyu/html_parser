#pragma once
#include <string_view>

namespace hps {
class Document;
class Element;
class ElementQuery;

class Query {
  public:
    static ElementQuery css(const Element& element, std::string_view selector);
    static ElementQuery css(const Document& document, std::string_view selector);

    static ElementQuery xpath(const Element& element, std::string_view expression);
    static ElementQuery xpath(const Document& document, std::string_view expression);
};
}  // namespace hps