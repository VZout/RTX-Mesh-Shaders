/*!
 *  \author    Viktor Zoutman
 *  \date      2019-2020
 *  \copyright GNU General Public License v3.0
 */

#pragma once

//! Used to check whether a method exists in a class.
#define DEFINE_HAS_METHOD(f) \
template<typename, typename T> \
struct HasMethod_##f { \
    static_assert( \
        std::integral_constant<T, false>::value, \
        "Second template parameter needs to be of function type."); \
}; \
 \
template<typename C, typename Ret, typename... Args> \
struct HasMethod_##f<C, Ret(Args...)> { \
private: \
    template<typename T> \
    static constexpr auto Check(T*) \
    -> typename \
        std::is_same< \
            decltype( std::declval<T>().f( std::declval<Args>()... ) ), \
            Ret \
        >::type; \
 \
    template<typename> \
    static constexpr std::false_type Check(...); \
 \
    typedef decltype(Check<C>(0)) type; \
 \
public: \
    static constexpr bool value = type::value; \
};

DEFINE_HAS_METHOD(GetInputLayout)

#define IS_PROPER_VERTEX_CLASS(type) static_assert(HasMethod_GetInputLayout<type, gfx::PipelineState::InputLayout()>::value, "Could not locate the required type::GetInputLayout function. If intelisense gives you this error ignore it.");
