#ifndef ARGON_MULTIOPTION_INCLUDE
#define ARGON_MULTIOPTION_INCLUDE

#include "Argon/Flag.hpp"
#include "Argon/Options/OptionCharBase.hpp"
#include "Argon/Options/OptionComponent.hpp"
#include "Argon/Options/SetValue.hpp"

// Template Specializations

namespace Argon {
    template <typename T>
    class MultiOption;

    class IsMultiOption {};

    class IArrayCapacity {
    protected:
        size_t m_maxSize;
        size_t m_nextIndex = 0;

        explicit IArrayCapacity(size_t maxSize);

        virtual ~IArrayCapacity() = default;
    public:
        [[nodiscard]] auto getMaxSize() const -> size_t;

        [[nodiscard]] auto isAtMaxCapacity() const -> bool;
    };

    // MultiOptionArrayBase

    template <typename Derived, typename T, size_t N>
    class MultiOptionArrayBase
        : public HasFlag<Derived>, public ISetValue,
          public OptionComponent<Derived>, public IArrayCapacity,
          public Converter<Derived, T>, public IsMultiOption {
    protected:
        std::array<T, N> m_values;
        std::array<T, N>* m_out = nullptr;
        bool m_maxCapacityError = false;

        MultiOptionArrayBase();

        explicit MultiOptionArrayBase(const std::array<T, N>& defaultValue);

        explicit MultiOptionArrayBase(std::array<T, N> *out);

        MultiOptionArrayBase(const std::array<T, N>& defaultValue, const std::array<T, N> *out);
    public:
        auto getValue() const -> const std::array<T, N>&;
    protected:
        auto setValue(const IOptionConfig& optionConfig, std::string_view flag, std::string_view value) -> void override;
    };

    // MultiOption with std::array<T, N>

    template<typename T, size_t N>
    class MultiOption<std::array<T, N>>
        : public MultiOptionArrayBase<MultiOption<std::array<T, N>>, T, N>,
          public OptionTypeExtensions<MultiOption<std::array<T, N>>, T> {
        using ISetValue::setValue;
    public:
        MultiOption() = default;

        explicit MultiOption(const std::array<T, N>& defaultValue);

        explicit MultiOption(std::array<T, N> *out);

        MultiOption(const std::array<T, N>& defaultValue, const std::array<T, N> *out);

        auto setValue(const ParserConfig& parserConfig, std::string_view flag, std::string_view value) -> void override;
    };

    // MultiOptionVectorBase

    template <typename Derived, typename T>
    class MultiOptionVectorBase
        : public HasFlag<Derived>, public ISetValue,
          public OptionComponent<Derived>,
          public IsMultiOption, public Converter<Derived, T> {
    protected:
        std::vector<T> m_values;
        std::vector<T>* m_out = nullptr;

        MultiOptionVectorBase() = default;

        explicit MultiOptionVectorBase(const std::vector<T>& defaultValue);

        explicit MultiOptionVectorBase(std::vector<T> *out);

        MultiOptionVectorBase(const std::vector<T>& defaultValue, std::vector<T> *out);
    public:
        auto getValue() const -> const std::vector<T>&;
    protected:
        auto setValue(const IOptionConfig& optionConfig, std::string_view flag, std::string_view value) -> void override;
    };

    // MultiOption with std::vector

    template<typename T>
    class MultiOption<std::vector<T>>
        : public MultiOptionVectorBase<MultiOption<std::vector<T>>, T>,
          public OptionTypeExtensions<MultiOption<std::vector<T>>, T> {
        using ISetValue::setValue;
    public:
        MultiOption() = default;

        explicit MultiOption(const std::vector<T>& defaultValue);

        explicit MultiOption(std::vector<T> *out);

        MultiOption(const std::vector<T>& defaultValue, std::vector<T> *out);

        auto setValue(const ParserConfig& parserConfig, std::string_view flag, std::string_view value) -> void override;
    };
}

// Deduction guides

namespace Argon {
    // Deduction guides for std::array
    
    template <typename T, std::size_t N>
    MultiOption(std::array<T, N>*) -> MultiOption<std::array<T, N>>;
    
    // Deduction guides for std::vector

    template <typename T>
    MultiOption(std::vector<T>*) -> MultiOption<std::vector<T>>;
}

// Template Implementations

namespace Argon {
// IArrayCapacity

inline IArrayCapacity::IArrayCapacity(const size_t maxSize) : m_maxSize(maxSize) {}

inline auto IArrayCapacity::getMaxSize() const -> size_t {
    return m_maxSize;
}

inline auto IArrayCapacity::isAtMaxCapacity() const -> bool {
    return m_nextIndex == m_maxSize;
}

// MultiOptionArrayBase

template<typename Derived, typename T, size_t N>
MultiOptionArrayBase<Derived, T, N>::MultiOptionArrayBase() : IArrayCapacity(N) {}

template<typename Derived, typename T, size_t N>
MultiOptionArrayBase<Derived, T, N>::MultiOptionArrayBase(const std::array<T, N>& defaultValue)
    : IArrayCapacity(N), m_values(defaultValue) {}

template<typename Derived, typename T, size_t N>
MultiOptionArrayBase<Derived, T, N>::MultiOptionArrayBase(std::array<T, N>* out) : IArrayCapacity(N), m_out(out) {}

template<typename Derived, typename T, size_t N>
MultiOptionArrayBase<Derived, T, N>::MultiOptionArrayBase(const std::array<T, N>& defaultValue, const std::array<T, N> *out)
    : IArrayCapacity(N), m_values(defaultValue), m_out(out) {
    if (m_out != nullptr) {
        *m_out = m_values;
    }
}

template<typename Derived, typename T, size_t N>
auto MultiOptionArrayBase<Derived, T, N>::getValue() const -> const std::array<T, N>& {
    return m_values;
}

template<typename Derived, typename T, size_t N>
auto MultiOptionArrayBase<Derived, T, N>::setValue(
        const IOptionConfig& optionConfig, std::string_view flag, std::string_view value
    ) -> void {
    if (this->m_maxCapacityError) {
        this->m_error.clear();
        return;
    }

    if (m_nextIndex >= N) {
        this->m_error = std::format("Flag '{}' only supports a maximum of {} values", flag, N);
        m_maxCapacityError = true;
        return;
    }

    this->convert(static_cast<const OptionConfig<T>&>(optionConfig), flag, value, m_values[m_nextIndex]);
    if (this->hasConversionError()) {
        this->m_error = this->getConversionError();
        return;
    }
    if (m_out != nullptr) {
        (*m_out)[m_nextIndex] = m_values[m_nextIndex];
    }
    m_nextIndex++;
    this->m_isSet = true;
}

// MultiOption with std::array

template<typename T, size_t N>
MultiOption<std::array<T, N>>::MultiOption(const std::array<T, N>& defaultValue)
    : MultiOptionArrayBase<MultiOption, T, N>(defaultValue) {}

template<typename T, size_t N>
MultiOption<std::array<T, N>>::MultiOption(std::array<T, N> *out)
    : MultiOptionArrayBase<MultiOption, T, N>(out) {}

template<typename T, size_t N>
MultiOption<std::array<T, N>>::MultiOption(const std::array<T, N>& defaultValue, const std::array<T, N> *out)
    : MultiOptionArrayBase<MultiOption, T, N>(defaultValue, out) {}

template<typename T, size_t N>
auto MultiOption<std::array<T, N>>::setValue(const ParserConfig& parserConfig, std::string_view flag, std::string_view value) -> void {
    OptionConfig<T> optionConfig = detail::getOptionConfig<MultiOption, T>(parserConfig, this);
    MultiOptionArrayBase<MultiOption, T, N>::setValue(optionConfig, flag, value);
    this->m_isSet = true;
}

// MultiOptionVectorBase

template<typename Derived, typename T>
MultiOptionVectorBase<Derived, T>::MultiOptionVectorBase(const std::vector<T>& defaultValue)
    : m_values(defaultValue) {}

template<typename Derived, typename T>
MultiOptionVectorBase<Derived, T>::MultiOptionVectorBase(std::vector<T> *out)
    : m_out(out) {}

template<typename Derived, typename T>
MultiOptionVectorBase<Derived, T>::MultiOptionVectorBase(const std::vector<T>& defaultValue, std::vector<T> *out)
    : m_values(defaultValue), m_out(out) {
    if (out != nullptr) {
        *m_out = m_values;
    }
}

template<typename Derived, typename T>
auto MultiOptionVectorBase<Derived, T>::getValue() const -> const std::vector<T>& {
    return m_values;
}

template<typename Derived, typename T>
auto MultiOptionVectorBase<Derived, T>::setValue(
        const IOptionConfig& optionConfig, std::string_view flag, std::string_view value
    ) -> void {
    T temp;
    this->convert(static_cast<const OptionConfig<T>&>(optionConfig), flag, value, temp);
    if (this->hasConversionError()) {
        this->m_error = this->getConversionError();
        return;
    }
    if (!this->m_isSet) {
        m_values.clear();
    }
    m_values.push_back(temp);
    if (m_out != nullptr) {
        m_out->push_back(temp);
    }
    this->m_isSet = true;
}

// MultiOption with std::vector<T> (where T is a numeric char)

template<typename T>
MultiOption<std::vector<T>>::MultiOption(const std::vector<T>& defaultValue)
    : MultiOptionVectorBase<MultiOption, T>(defaultValue) {}

template <typename T>
MultiOption<std::vector<T>>::MultiOption(std::vector<T>* out)
    : MultiOptionVectorBase<MultiOption, T>(out) {}

template<typename T>
MultiOption<std::vector<T>>::MultiOption(const std::vector<T>& defaultValue, std::vector<T> *out)
    : MultiOptionVectorBase<MultiOption, T>(defaultValue, out) {}

template <typename T>
void MultiOption<std::vector<T>>::setValue(const ParserConfig& parserConfig,
    const std::string_view flag, const std::string_view value) {
    OptionConfig<T> optionConfig = detail::getOptionConfig<MultiOption, T>(parserConfig, this);
    MultiOptionVectorBase<MultiOption, T>::setValue(optionConfig, flag, value);
    this->m_isSet = true;
}

} // End namespace Argon

#endif // ARGON_MULTIOPTION_INCLUDE