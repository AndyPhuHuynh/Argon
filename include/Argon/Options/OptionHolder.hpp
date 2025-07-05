#ifndef ARGON_OPTION_HOLDER_INCLUDE
#define ARGON_OPTION_HOLDER_INCLUDE

#include <cassert>
#include <memory>

#include "Argon/Options/IOption.hpp"

namespace Argon {

template <typename OptionType>
class OptionHolder {
    std::unique_ptr<OptionType> m_ownedOption = nullptr;
    OptionType *m_externalOption = nullptr;

public:
    OptionHolder() = default;

    explicit OptionHolder(OptionType& opt) : m_externalOption(&opt) {}

    explicit OptionHolder(OptionType&& opt) : m_ownedOption(opt.clone()) {}

    OptionHolder(const OptionHolder& other) {
        if (other.m_ownedOption) {
            m_ownedOption = other.m_ownedOption->clone();
        }
        m_externalOption = other.m_externalOption;
    }

    auto operator=(const OptionHolder& other) -> OptionHolder& {
        if (this != &other) {
            if (other.m_ownedOption) {
                m_ownedOption = other.m_ownedOption->clone();
            }
            m_externalOption = other.m_externalOption;
        }
        return *this;
    }

    OptionHolder(OptionHolder&& other) noexcept = default;

    auto operator=(OptionHolder&& other) noexcept -> OptionHolder& = default;

    [[nodiscard]] auto copyAsOwned() const -> OptionHolder {
        OptionHolder newHolder;
        if (m_ownedOption) {
            newHolder.m_ownedOption = m_ownedOption->clone();
        } else if (m_externalOption) {
            newHolder.m_ownedOption = m_externalOption->clone();
        } else {
            assert(false && "Attempting to copy an empty OptionHolder");
        }
        return newHolder;
    }

    [[nodiscard]] auto getRef() -> IOption& {
        assert((m_ownedOption != nullptr || m_externalOption != nullptr)
            && "Attempting to get use getRef without a valid reference");
        return m_ownedOption ? *m_ownedOption : *m_externalOption;
    }

    [[nodiscard]] auto getRef() const -> const IOption& {
        assert((m_ownedOption != nullptr || m_externalOption != nullptr)
            && "Attempting to get use getRef without a valid reference");
        return m_ownedOption ? *m_ownedOption : *m_externalOption;
    }

    [[nodiscard]] auto getPtr() -> IOption * {
        return m_ownedOption ? m_ownedOption.get() : m_externalOption;
    }

    [[nodiscard]] auto getPtr() const -> const IOption * {
        return m_ownedOption ? m_ownedOption.get() : m_externalOption;
    }
};

} // End namespace Argon

#endif // ARGON_OPTION_HOLDER_INCLUDE
