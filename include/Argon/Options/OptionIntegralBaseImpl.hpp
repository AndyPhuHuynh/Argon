#ifndef ARGON_OPTION_INTEGRAL_BASE_INCLUDE
#define ARGON_OPTION_INTEGRAL_BASE_INCLUDE

#include <optional>

namespace Argon {
    template <typename Integral>
    class OptionIntegralBase {
    protected:
        std::optional<Integral> m_min;
        std::optional<Integral> m_max;

        OptionIntegralBase() = default;
        virtual ~OptionIntegralBase() = default;
    public:
        [[nodiscard]] auto getMin() const -> std::optional<Integral> {
            return m_min;
        }

        [[nodiscard]] auto getMax() const -> std::optional<Integral> {
            return m_max;
        }
    };

    template <typename Derived, typename Integral>
    class OptionIntegralBaseImpl : public OptionIntegralBase<Integral> {
    public:
        auto withMin(Integral min) & -> Derived& {
            if (this->m_max.has_value() && this->m_max < min) {
                throw std::invalid_argument("Parser config error: min must be less than max");
            }
            this->m_min = min;
            return static_cast<Derived&>(*this);
        }

        auto withMin(Integral min) && -> Derived&& {
            if (this->m_max.has_value() && this->m_max < min) {
                throw std::invalid_argument("Parser config error: min must be less than max");
            }
            this->m_min = min;
            return static_cast<Derived&&>(*this);
        }

        auto withMax(Integral max) & -> Derived& {
            if (this->m_min.has_value() && this->m_min > max) {
                throw std::invalid_argument("Parser config error: max must be greater than min");
            }
            this->m_max = max;
            return static_cast<Derived&>(*this);
        }

        auto withMax(Integral max) && -> Derived&& {
            if (this->m_min.has_value() && this->m_min > max) {
                throw std::invalid_argument("Parser config error: max must be greater than min");
            }
            this->m_max = max;
            return static_cast<Derived&&>(*this);
        }
    };
}

#endif // ARGON_OPTION_INTEGRAL_BASE_INCLUDE
