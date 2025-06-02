#ifndef ARGON_ATTRIBUTES_INCLUDE
#define ARGON_ATTRIBUTES_INCLUDE

#include <algorithm>
#include <ranges>
#include <string>
#include <string_view>
#include <vector>

namespace Argon {
    class Constraints {
        struct MutuallyExclusive {
            std::string flag;
            std::vector<std::string> exclusiveFlags;
        };

    public:
        auto require(std::string_view flag) -> void;

        auto mutuallyExclusive(std::string_view flag, std::initializer_list<std::string_view> exclusiveFlags) -> void;

    private:
        std::vector<std::string> m_requiredOptions;
        std::vector<MutuallyExclusive> m_mutuallyExclusiveOptions;
    };

}

//---------------------------------------------------Implementations----------------------------------------------------

#include "Context.hpp"
#include "Option.hpp"

namespace Argon {

inline auto Constraints::require(std::string_view flag) -> void {
    if (std::ranges::contains(m_requiredOptions, flag)) return;
    m_requiredOptions.emplace_back(flag);
}

inline auto Constraints::mutuallyExclusive(const std::string_view flag, const std::initializer_list<std::string_view> exclusiveFlags) -> void {
    const auto it = std::ranges::find_if(m_mutuallyExclusiveOptions, [&flag](const auto& me) {
        return me.flag == flag;
    });

    // Flag already added to list
    if (it != m_mutuallyExclusiveOptions.end()) {
        auto& [meFlag, meOtherFlags] = *it;
        for (const auto exclusiveFlag : exclusiveFlags) {
            if (!std::ranges::contains(meOtherFlags, exclusiveFlag)) {
                meOtherFlags.emplace_back(exclusiveFlag);
            }
        }
        return;
    }

    // Flag not added in list yet
    auto& [meFlag, meOtherFlags] = m_mutuallyExclusiveOptions.emplace_back();
    meFlag = flag;
    meOtherFlags.reserve(exclusiveFlags.size());
    for (const auto& exclusiveFlag : exclusiveFlags) {
        meOtherFlags.emplace_back(exclusiveFlag);
    }
}

} // End namespace Argon

#endif
