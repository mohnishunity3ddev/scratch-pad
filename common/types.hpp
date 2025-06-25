#pragma once 

#include <type_traits>
#include <concepts>

template<typename T>
concept podtype = std::is_trivial_v<T> && std::is_standard_layout_v<T>;