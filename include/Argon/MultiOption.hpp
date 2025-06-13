#ifndef ARGON_MULTIOPTION_INCLUDE
#define ARGON_MULTIOPTION_INCLUDE

#include "Option.hpp" // NOLINT (misc-unused-include)

// Template Specializations

namespace Argon {
    template <typename T>
    class MultiOption;

    class IsMultiOption {};
    
    // MultiOption with std::array

    class MultiOptionStdArrayBase : public IsMultiOption {
    protected:
        size_t m_maxSize;
        size_t m_nextIndex = 0;

        explicit MultiOptionStdArrayBase(size_t maxSize);
    public:
        [[nodiscard]] auto getMaxSize() const -> size_t;

        [[nodiscard]] auto isAtMaxCapacity() const -> bool;
    };

    template<typename T, size_t N>
    class MultiOption<std::array<T, N>> : public OptionBase, public OptionComponent<MultiOption<std::array<T, N>>>,
                                          public MultiOptionStdArrayBase, public Converter<MultiOption<std::array<T, N>>, T>{
        std::array<T, N> m_values;
        std::array<T, N>* m_out = nullptr;
        bool m_maxCapacityError = false;
    public:
        MultiOption();

        explicit MultiOption(const std::array<T, N>& defaultValue);

        explicit MultiOption(std::array<T, N> *out);

        MultiOption(const std::array<T, N>& defaultValue, const std::array<T, N> *out);

        auto getValue() const -> const std::array<T, N>&;

        auto setValue(const DefaultConversions& conversions, const std::string& flag, const std::string& value) -> void override;
    };

    // MultiOption with std::vector

    template<typename T>
    class MultiOption<std::vector<T>> : public OptionBase, public OptionComponent<MultiOption<std::vector<T>>>,
                                        public IsMultiOption, public Converter<MultiOption<std::vector<T>>, T> {
        std::vector<T> m_values;
        std::vector<T>* m_out = nullptr;
    public:
        MultiOption() = default;

        explicit MultiOption(const std::vector<T>& defaultValue);

        explicit MultiOption(std::vector<T> *out);

        MultiOption(const std::vector<T>& defaultValue, std::vector<T> *out);

        auto getValue() const -> const std::vector<T>&;

        auto setValue(const DefaultConversions& conversions, const std::string& flag, const std::string& value) -> void override;
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
    // MultiOptionStdArrayBase

    inline MultiOptionStdArrayBase::MultiOptionStdArrayBase(const size_t maxSize) : m_maxSize(maxSize) {}

    inline auto MultiOptionStdArrayBase::getMaxSize() const -> size_t {
        return m_maxSize;
    }

    inline auto MultiOptionStdArrayBase::isAtMaxCapacity() const -> bool {
        return m_nextIndex == m_maxSize;
    }

    // MultiOption with std::array

    template<typename T, size_t N>
    MultiOption<std::array<T, N>>::MultiOption() : MultiOptionStdArrayBase(N) {}

    template<typename T, size_t N>
    MultiOption<std::array<T, N>>::MultiOption(const std::array<T, N>& defaultValue)
        : MultiOptionStdArrayBase(N), m_values(defaultValue) {}

    template <typename T, size_t N>
    MultiOption<std::array<T, N>>::MultiOption(std::array<T, N>* out) : MultiOptionStdArrayBase(N), m_out(out) {}

    template<typename T, size_t N>
    MultiOption<std::array<T, N>>::MultiOption(const std::array<T, N>& defaultValue, const std::array<T, N> *out)
        : MultiOptionStdArrayBase(N), m_values(defaultValue), m_out(out) {
        *m_out = m_values;
    }

    template<typename T, size_t N>
    auto MultiOption<std::array<T, N>>::getValue() const -> const std::array<T, N>& {
        return m_values;
    }

    template <typename T, size_t N>
    void MultiOption<std::array<T, N>>::setValue(const DefaultConversions& conversions, const std::string& flag, const std::string& value) {
        if (m_maxCapacityError) {
            this->m_error.clear();
            return;
        }
        
        if (m_nextIndex >= N) {
            this->m_error = std::format("Flag '{}' only supports a maximum of {} values", flag, N);
            m_maxCapacityError = true;
            return;
        }

        this->convert(conversions, flag, value, m_values[m_nextIndex]);
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

    // MultiOption with std::vector

    template<typename T>
    MultiOption(std::vector<T>) -> MultiOption<std::vector<T>>;

    template<typename T>
    MultiOption<std::vector<T>>::MultiOption(const std::vector<T>& defaultValue) : m_values(defaultValue) {}

    template <typename T>
    MultiOption<std::vector<T>>::MultiOption(std::vector<T>* out) : m_out(out) {}

    template<typename T>
    MultiOption<std::vector<T>>::MultiOption(const std::vector<T>& defaultValue, std::vector<T> *out)
        : m_values(defaultValue), m_out(out) {
        *m_out = m_values;
    }

    template<typename T>
    auto MultiOption<std::vector<T>>::getValue() const -> const std::vector<T>& {
        return m_values;
    }

    template <typename T>
    void MultiOption<std::vector<T>>::setValue(const DefaultConversions& conversions, const std::string& flag, const std::string& value) {
        T temp;
        this->convert(conversions, flag, value, temp);
        if (this->hasConversionError()) {
            this->m_error = this->getConversionError();
            return;
        }
        m_values.push_back(temp);
        if (m_out != nullptr) {
            m_out->push_back(temp);
        }
        this->m_isSet = true;
    }
}

#endif // ARGON_MULTIOPTION_INCLUDE