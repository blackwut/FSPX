#ifndef __CONNECTORS_HPP__
#define __CONNECTORS_HPP__

#if defined(__DEBUG__CONNECTORS__)
#include <typeinfo>
template <typename T>
void print_debug(const std::string name, const std::string message, T t) {

    std::cout << "[" name << "] " << message << " ";

    if constexpr (!std::is_member_function_pointer<decltype(&T::print_debug())>::value) {
        std::cout << typeid(t).name() << " has not print_debug() member function!" std::endl;
    } else {
        t.print_debug();
    }
}
#endif // __DEBUG__CONNECTORS__

#include "generic.hpp"
#include "memory.hpp"
#include "all2all.hpp"

#endif // __CONNECTORS_HPP__