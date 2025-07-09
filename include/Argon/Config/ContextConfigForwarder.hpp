#ifndef ARGON_CONTEXT_CONFIG_FORWARDER_INCLUDE
#define ARGON_CONTEXT_CONFIG_FORWARDER_INCLUDE

#include "Argon/Config/ContextConfig.hpp"

namespace Argon::detail {

template <typename Derived>
class ContextConfigForwarder {
protected:
    ContextConfigForwarder() = default;
    ContextConfigForwarder(const ContextConfigForwarder &) = default;
    ContextConfigForwarder& operator=(const ContextConfigForwarder &) = default;
    ContextConfigForwarder(ContextConfigForwarder &&) = default;
    ContextConfigForwarder& operator=(ContextConfigForwarder &&) = default;
    virtual ~ContextConfigForwarder() = default;

    virtual auto getConfigImpl() -> ContextConfig& = 0;
public:
    [[nodiscard]] auto getDefaultCharMode() const -> CharMode {
        return getConfigImpl().getDefaultCharMode();
    }

    auto setDefaultCharMode(const CharMode newCharMode) & -> Derived& {
        getConfigImpl().setDefaultCharMode(newCharMode);
        return static_cast<Derived&>(*this);
    }

    auto setDefaultCharMode(const CharMode newCharMode) && -> Derived&& {
        getConfigImpl().setDefaultCharMode(newCharMode);
        return static_cast<Derived&&>(*this);
    }

    [[nodiscard]] auto getDefaultPositionalPolicy() const -> PositionalPolicy {
        return getConfigImpl().getDefaultPositionalPolicy();
    }

    auto setDefaultPositionalPolicy(const PositionalPolicy newPolicy) & -> Derived& {
        getConfigImpl().setDefaultPositionalPolicy(newPolicy);
        return static_cast<Derived&>(*this);
    }

    auto setDefaultPositionalPolicy(const PositionalPolicy newPolicy) && -> Derived&& {
        getConfigImpl().setDefaultPositionalPolicy(newPolicy);
        return static_cast<Derived&&>(*this);
    }

    [[nodiscard]] auto getDefaultConversions() const -> const DefaultConversions& {
        return getConfigImpl().getDefaultConversions();
    }

    template <typename T>
    auto registerConversionFn(const std::function<bool(std::string_view, T *)>& conversionFn) & -> Derived& {
        getConfigImpl().registerConversionFn(conversionFn);
        return static_cast<Derived&>(*this);
    }

    template <typename T>
    auto registerConversionFn(const std::function<bool(std::string_view, T *)>& conversionFn) && -> Derived&& {
        getConfigImpl().registerConversionFn(conversionFn);
        return static_cast<Derived&&>(*this);
    }

    template <typename T> requires is_non_bool_number<T>
    auto getMin() const -> T {
        return getConfigImpl().getMin<T>();
    }

    template <typename T> requires is_non_bool_number<T>
    auto setMin(T min) & -> Derived& {
        getConfigImpl().setMin<T>(min);
        return static_cast<Derived&>(this);
    }

    template <typename T> requires is_non_bool_number<T>
    auto setMin(T min) && -> Derived&& {
        getConfigImpl().setMin<T>(min);
        return static_cast<Derived&&>(this);
    }

    template <typename T> requires is_non_bool_number<T>
    auto getMax() const -> T {
        return getConfigImpl().getMax<T>();
    }

    template <typename T> requires is_non_bool_number<T>
    auto setMax(T max) & -> Derived& {
        getConfigImpl().setMax<T>(max);
        return static_cast<Derived&>(this);
    }

    template <typename T> requires is_non_bool_number<T>
    auto setMax(T max) && -> Derived&& {
        getConfigImpl().setMax<T>(max);
        return static_cast<Derived&&>(this);
    }
};

}
#endif // ARGON_CONTEXT_CONFIG_FORWARDER_INCLUDE
