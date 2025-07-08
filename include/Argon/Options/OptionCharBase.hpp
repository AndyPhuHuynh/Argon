#ifndef ARGON_OPTION_CHAR_BASE_INCLUDE
#define ARGON_OPTION_CHAR_BASE_INCLUDE

#include "Argon/ParserConfig.hpp"

namespace Argon {
    template <typename Derived>
    class OptionCharBase {
    protected:
        CharMode m_charMode = CharMode::UseDefault;
        OptionCharBase() = default;
        virtual ~OptionCharBase() = default;
    public:
        [[nodiscard]] auto getCharMode() const -> CharMode {
            return m_charMode;
        }

        auto setCharMode(const CharMode mode) & -> Derived& {
            m_charMode = mode;
            return *this;
        }

        auto setCharMode(const CharMode mode) && -> Derived&& {
            m_charMode = mode;
            return static_cast<Derived&&>(*this);
        }
    };
}
#endif // ARGON_OPTION_CHAR_BASE_INCLUDE
