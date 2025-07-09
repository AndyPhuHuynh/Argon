#ifndef ARGON_CONFIG_TYPES_INCLUDE
#define ARGON_CONFIG_TYPES_INCLUDE

#include <functional>
#include <memory>
#include <string_view>
#include <typeindex>

#include "Argon/Traits.hpp"

namespace Argon {

using DefaultConversionFn = std::function<bool(std::string_view, void*)>;
using DefaultConversions  = std::unordered_map<std::type_index, DefaultConversionFn>;

enum class CharMode {
    UseDefault = 0,
    ExpectAscii,
    ExpectInteger,
};

enum class PositionalPolicy {
    UseDefault = 0,
    Interleaved,
    BeforeFlags,
    AfterFlags,
};

} // End namespace Argon

namespace Argon::detail {

struct IntegralBoundsBase {
    IntegralBoundsBase() = default;
    virtual ~IntegralBoundsBase() = default;
    [[nodiscard]] virtual auto clone() const -> std::unique_ptr<IntegralBoundsBase> = 0;
};

template <typename T> requires is_non_bool_number<T>
struct IntegralBounds final : IntegralBoundsBase{
    T min = std::numeric_limits<T>::lowest();
    T max = std::numeric_limits<T>::max();

    [[nodiscard]] auto clone() const -> std::unique_ptr<IntegralBoundsBase> override;
};

class BoundsMap {
public:
    std::unordered_map<std::type_index, std::unique_ptr<IntegralBoundsBase>> map;

    BoundsMap() = default;
    BoundsMap(const BoundsMap&);
    BoundsMap& operator=(const BoundsMap&);
    BoundsMap(BoundsMap&&) noexcept = default;
    BoundsMap& operator=(BoundsMap&&) noexcept = default;
};

} // End namespace Argon::detail

namespace Argon::detail {

template<typename T> requires is_non_bool_number<T>
auto IntegralBounds<T>::clone() const -> std::unique_ptr<IntegralBoundsBase> {
    return std::make_unique<IntegralBounds>(*this);
}


inline BoundsMap::BoundsMap(const BoundsMap& other) {
    for (const auto& [typeIndex, bound] : other.map) {
        map[typeIndex] = bound->clone();
    }
}

inline BoundsMap& BoundsMap::operator=(const BoundsMap& other) {
    if (this != &other) {
        for (const auto& [typeIndex, bound] : other.map) {
            map[typeIndex] = bound->clone();
        }
    }
    return *this;
}

} // End namespace Argon::detail

#endif // ARGON_CONFIG_TYPES_INCLUDE
