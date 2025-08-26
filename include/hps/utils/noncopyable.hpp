#pragma once

/**
 * @brief 不可拷贝基类，用于禁止类的拷贝构造和拷贝赋值
 *
 * 继承此类的类型将自动禁用拷贝语义，只能移动或引用
 */

namespace hps {
class NonCopyable {
  protected:
    constexpr NonCopyable() = default;
    ~NonCopyable()          = default;

    // 允许移动
    NonCopyable(NonCopyable&&)            = default;
    NonCopyable& operator=(NonCopyable&&) = default;

    // 禁止拷贝
    NonCopyable(const NonCopyable&)            = delete;
    NonCopyable& operator=(const NonCopyable&) = delete;
};
}  // namespace hps