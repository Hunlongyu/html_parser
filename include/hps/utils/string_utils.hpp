#pragma once

#include <concepts>
#include <string>
#include <string_view>

namespace hps {

// 字符串类型约束
template <typename T>
concept StringLike = std::convertible_to<T, std::string_view>;

}  // namespace hps