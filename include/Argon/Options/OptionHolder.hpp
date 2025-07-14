#ifndef ARGON_OPTION_HOLDER_INCLUDE
#define ARGON_OPTION_HOLDER_INCLUDE

#include <cassert>
#include <memory>

#include <Argon/Options/IOption.hpp>

namespace Argon {

template <typename OptionType>
class OptionHolder {
    std::unique_ptr<IOption> m_ownedOption = nullptr;
    IOption *m_externalOption = nullptr;

public:
    OptionHolder() = default;

    explicit OptionHolder(IOption& opt) : m_externalOption(&opt) {
        if (!dynamic_cast<OptionType *>(&opt)) {
            throw std::runtime_error("OptionHolder was given an incorrect type");
        }
    }

    explicit OptionHolder(IOption&& opt) : m_ownedOption(opt.clone()) {
        if (!dynamic_cast<OptionType *>(&opt)) {
            throw std::runtime_error("OptionHolder was given an incorrect type");
        }
    }

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

    [[nodiscard]] auto getRef() -> OptionType& {
        if (m_ownedOption != nullptr) {
            return dynamic_cast<OptionType&>(*m_ownedOption);
        }
        if (m_externalOption != nullptr) {
            return dynamic_cast<OptionType&>(*m_externalOption);
        }
        throw std::runtime_error("Unable to get reference of a null OptionHolder");
    }

    [[nodiscard]] auto getRef() const -> const OptionType& {
        if (m_ownedOption != nullptr) {
            return dynamic_cast<OptionType&>(*m_ownedOption);
        }
        if (m_externalOption != nullptr) {
            return dynamic_cast<OptionType&>(*m_externalOption);
        }
        throw std::runtime_error("Unable to get reference of a null OptionHolder");
    }

    [[nodiscard]] auto getPtr() -> OptionType * {
        return dynamic_cast<OptionType *>(m_ownedOption ? m_ownedOption.get() : m_externalOption);
    }

    [[nodiscard]] auto getPtr() const -> const OptionType * {
        return dynamic_cast<OptionType *>(m_ownedOption ? m_ownedOption.get() : m_externalOption);
    }
};

} // End namespace Argon

#endif // ARGON_OPTION_HOLDER_INCLUDE
