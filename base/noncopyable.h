// clang-format off
#pragma once

namespace mudlite
{
    
/**
 * noncopyable被继承后, 派生类对象可以正常地构造和析构
 * 但是派生类对象不可进行拷贝构造和拷贝赋值
 */
class noncopyable
{
public:
    noncopyable(const noncopyable &) = delete;
    noncopyable &operator=(const noncopyable &) = delete;
protected:
    noncopyable() = default;
    ~noncopyable() = default;
};

} // namespace mudlite