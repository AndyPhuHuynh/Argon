#ifndef ARGON_OPTION_HOLDER_INCLUDE
#define ARGON_OPTION_HOLDER_INCLUDE

#include <cassert>
#include <memory>

#include "Argon/Options/IOption.hpp"

namespace Argon {

class OptionHolder {
    std::unique_ptr<IOption> m_ownedOption = nullptr;
    IOption *m_externalOption = nullptr;

public:
    OptionHolder() = default;

    explicit OptionHolder(IOption& opt) : m_externalOption(&opt) {}

    explicit OptionHolder(IOption&& opt) : m_ownedOption(opt.clone()) {}

    OptionHolder(const OptionHolder& other) {
        if (other.m_ownedOption) {
            m_ownedOption = other.m_ownedOption->clone();
        } else if (other.m_externalOption) {
            m_ownedOption = other.m_externalOption->clone();
        } else {
            assert(false && "Attempting to copy an empty OptionHolder");
        }
        m_externalOption = nullptr;
    }

    auto operator=(const OptionHolder& other) -> OptionHolder& {
        if (this != &other) {
            if (other.m_ownedOption) {
                m_ownedOption = other.m_ownedOption->clone();
            } else if (other.m_externalOption) {
                m_ownedOption = other.m_externalOption->clone();
            } else {
                assert(false && "Attempting to copy an empty OptionHolder");
            }
            m_externalOption = nullptr;
        }
        return *this;
    }

    OptionHolder(OptionHolder&& other) noexcept = default;
    auto operator=(OptionHolder&& other) noexcept -> OptionHolder& = default;

    [[nodiscard]] auto getRef() -> IOption& {
        assert(m_ownedOption || m_externalOption);
        return m_ownedOption ? *m_ownedOption : *m_externalOption;
    }

    [[nodiscard]] auto getRef() const -> const IOption& {
        assert(m_ownedOption || m_externalOption);
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
