#ifndef ARGON_OPTION_COMPONENT_INCLUDE
#define ARGON_OPTION_COMPONENT_INCLUDE

#include <Argon/Options/IOption.hpp>

namespace Argon {
template <typename Derived>
class OptionComponent : public IOption {
protected:
    OptionComponent() = default;
public:
    [[nodiscard]] auto clone() const -> std::unique_ptr<IOption> override {
        return std::make_unique<Derived>(static_cast<const Derived&>(*this));
    }

    auto description(const std::string_view desc) & -> Derived& {
        m_description = desc;
        return static_cast<Derived&>(*this);
    }

    auto description(const std::string_view desc) && -> Derived&& {
        m_description = desc;
        return static_cast<Derived&&>(*this);
    }

    auto description(const std::string_view inputHint, const std::string_view desc) & -> Derived& {
        m_inputHint = inputHint;
        m_description = desc;
        return static_cast<Derived&>(*this);
    }

    auto description(const std::string_view inputHint, const std::string_view desc) && -> Derived&& {
        m_inputHint = inputHint;
        m_description = desc;
        return static_cast<Derived&&>(*this);
    }

    auto operator()(const std::string_view desc) & -> Derived& {
        return static_cast<Derived&>(description(desc));
    }

    auto operator()(const std::string_view desc) && -> Derived&& {
        return static_cast<Derived&&>(description(desc));
    }

    auto operator()(const std::string_view inputHint, const std::string_view desc) & -> Derived& {
        return static_cast<Derived&>(description(inputHint, desc));
    }

    auto operator()(const std::string_view inputHint, const std::string_view desc) && -> Derived&& {
        return static_cast<Derived&&>(description(inputHint, desc));
    }
};

} // End namespace Argon

#endif // ARGON_OPTION_COMPONENT_INCLUDE
