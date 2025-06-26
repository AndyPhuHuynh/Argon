#ifndef ARGON_IOPTION_INCLUDE
#define ARGON_IOPTION_INCLUDE

#include <memory>
#include <string>

namespace Argon {
class IOption {
protected:
    std::string m_error;
    std::string m_inputHint;
    std::string m_description;

    bool m_isSet = false;
    IOption() = default;
public:
    IOption(const IOption&) = default;
    IOption& operator=(const IOption&) = default;

    IOption(IOption&&) noexcept = default;
    IOption& operator=(IOption&&) noexcept = default;

    virtual ~IOption() = default;

    [[nodiscard]] auto getError() const -> const std::string& {
        return m_error;
    }

    [[nodiscard]] auto hasError() const -> bool {
        return !m_error.empty();
    }

    [[nodiscard]] auto getInputHint() const -> const std::string& {
        return m_inputHint;
    }

    [[nodiscard]] auto getDescription() const -> const std::string& {
        return m_description;
    }

    auto clearError() -> void {
        m_error.clear();
    }

    [[nodiscard]] auto isSet() const -> bool {
        return m_isSet;
    }

    [[nodiscard]] virtual auto clone() const -> std::unique_ptr<IOption> = 0;
};

} // End namespace Argon
#endif // ARGON_IOPTION_INCLUDE
