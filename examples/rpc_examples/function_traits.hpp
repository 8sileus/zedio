#include <tuple>

template <typename T>
struct function_traits;

template <typename Ret, typename... Args>
struct function_traits<Ret(Args...)> {
    using return_type = Ret;
    using args_tuple_type = std::tuple<Args...>;
    static constexpr std::size_t arity = sizeof...(Args);

    template <std::size_t N>
    using arg = std::tuple_element_t<N, args_tuple_type>;
};

template <typename Ret, typename... Args>
struct function_traits<Ret (&)(Args...)> : public function_traits<Ret(Args...)> {};

template <typename Ret, typename... Args>
struct function_traits<Ret (*)(Args...)> : public function_traits<Ret(Args...)> {};
