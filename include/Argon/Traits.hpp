#pragma once
#include <type_traits>
#include <istream>

template<typename T, typename = void>
struct has_stream_extraction : std::false_type {};

template<typename T>
struct has_stream_extraction<T, std::void_t<
    decltype(std::declval<std::istream&>() >> std::declval<T&>())
>> : std::true_type {};


template<typename T>
std::string type_name() {
    return "unknown type";
}

template<>
inline std::string type_name<bool>() {
    return "boolean";
}

template<>
inline std::string type_name<char>() {
    return "char";
}

template<>
inline std::string type_name<signed char>() {
    return "signed char";
}

template<>
inline std::string type_name<unsigned char>() {
    return "unsigned char";
}

template<>
inline std::string type_name<short>() {
    return "short";
}

template<>
inline std::string type_name<unsigned short>() {
    return "unsigned short";
}

template<>
inline std::string type_name<int>() {
    return "integer";
}

template<>
inline std::string type_name<unsigned int>() {
    return "unsigned integer";
}

template<>
inline std::string type_name<long>() {
    return "long";
}

template<>
inline std::string type_name<unsigned long>() {
    return "unsigned long";
}

template<>
inline std::string type_name<long long>() {
    return "long long";
}

template<>
inline std::string type_name<unsigned long long>() {
    return "unsigned long long";
}


template<>
inline std::string type_name<float>() {
    return "floating point number";
}

template<>
inline std::string type_name<double>() {
    return "floating point number";
}

template<>
inline std::string type_name<long double>() {
    return "floating point number";
}

template<>
inline std::string type_name<std::string>() {
    return "string";
}
