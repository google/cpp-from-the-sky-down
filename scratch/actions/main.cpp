#include <iostream>
#include <ranges>
#include <type_traits>
#include <tuple>

namespace range::actions {

enum class processing_style {
  incremental,
  complete
};

template<typename aco>
concept incremental_input = aco::input_processing_style
    == processing_style::incremental;

template<typename aco>
concept complete_input = aco::input_processing_style
    == processing_style::complete;

template<typename aco>
concept incremental_output = aco::output_processing_style
    == processing_style::incremental;

template<typename aco>
concept complete_output = aco::output_processing_style
    == processing_style::complete;

template<typename aco, typename input, processing_style previous_ops>
struct action_closure_output_type {
  using type = typename aco::template output_type<input>;
};

template<incremental_input aco, typename input>
struct action_closure_output_type<aco, input, processing_style::complete> {
  using type = typename aco::template output_type<
      std::ranges::range_reference_t<std::remove_cvref_t<input>>>;
};

template<typename aco, typename input, processing_style previous_ops>
using action_closure_output_type_t = typename action_closure_output_type<aco,
                                                                         input,
                                                                         previous_ops>::type;
template<typename Child, processing_style ips, processing_style ops, typename Input, typename Next>
struct range_action_closure_object {
  [[no_unique_address]] Next next;

  using input_type = Input;
  using base = range_action_closure_object;

  constexpr static auto input_processing_style = ips;
  constexpr static auto output_processing_style = ops;

  constexpr range_action_closure_object(Next&& next) : next(std::move(next)) {}


  constexpr decltype(auto) finish()requires(incremental_input<
      Child>)
  {
    return next.finish();
  }

  constexpr bool done() const requires(incremental_input<
      Child>)
  {
    return next.done();
  }

  constexpr void process_incremental(input_type input)requires(incremental_input<
      Child>)
  {
    return next.process_incremental(static_cast<input_type>(input));
  }

  constexpr decltype(auto) process_complete(input_type input)requires(complete_input<
      Child>){
    return next.process_complete(static_cast<input_type>(input));

  }

  constexpr decltype(auto) process_complete(input_type input)requires(incremental_input<
      Child>)
  {
    for (int v: static_cast<input_type>(input)) {
      static_cast<Child&>(*this).process_incremental(std::forward<decltype(v)>(v));
    }

    static_cast<Child&>(*this).finish();

  }

};

struct void_tag {};
template<typename T>
struct instantiable {
  using type = T;
};

template<>
struct instantiable<void> {
  using type = void_tag;
};

template<typename T>
using instantiable_t = typename instantiable<T>::type;

template<template<typename, typename, typename...> typename aco,
    typename Parameters, typename... TypeParameters>
class range_action_closure_factory {
  [[no_unique_address]] instantiable_t<Parameters> parameters_;

 public:
  template<typename Input>
  using output_type = typename aco<Input,
                                   void_tag,
                                   TypeParameters...>::output_type;
  template<typename... Ts>
  constexpr explicit range_action_closure_factory(Ts&& ... ts)
      :parameters_{std::forward<Ts>(ts)...} {}

  template<typename Input, typename Next>
  constexpr auto make(Next&& next) {
    if constexpr (std::is_same_v<Parameters, void>) {
      return aco<Input, Next, TypeParameters...>{std::forward<Next>(next)};
    } else {
      return aco<Input, Next, TypeParameters...>{std::forward<Next>(next),
                                                 std::move(parameters_)};
    }
  }
};

namespace detail {

struct end_aco {
  template<typename T>
  constexpr decltype(auto) process_complete(T&& t) {
    return std::forward<T>(t);
  }
};

template<typename Previous>
struct end_factory {
  Previous& previous;

  template<typename Input, typename Next>
  constexpr auto make(Next&& next) {
    return previous.make(std::forward<Next>(next));
  }
};

struct end_factory_tag {};

struct empty {
  template<typename Next>
  constexpr auto make(Next&& next) {
    return std::forward<Next>(next);
  }
};

template<typename Input>
struct starting_factory {
  template<typename>
  using output_type = Input;

  template<typename, typename Next>
  constexpr auto make(Next&& next) {
    return std::forward<Next>(next);
  }
};
template<typename Input, typename Factory = starting_factory<Input>, typename Previous = empty>
struct input_factory {
  using output_type = typename Factory::template output_type<Input>;
  Factory& factory;
  Previous& previous;
  template<typename NewFactory>
  constexpr auto operator+(NewFactory&& new_factory) {
    return input_factory<output_type,
                         NewFactory,
                         input_factory>
        {new_factory, *this};
  }
  constexpr auto operator+(end_factory_tag) {
    return end_factory<input_factory>{*this};
  }

  template<typename Next>
  constexpr auto make(Next&& next) {
    return previous.make(factory.template make<Input>(
        std::forward<Next>(next)));
  }

};
}


template<typename Range, typename... Acos>
constexpr auto apply(Range&& range, Acos&& ... acos) {

  detail::empty empty;
  detail::starting_factory<decltype(range)> starting_factory;
  auto chain = (detail::input_factory<decltype(range)>{
      starting_factory, empty} + ... + std::forward<
      Acos>(acos)).make(detail::end_aco{});
  return chain.process_complete(std::forward<Range>(range));
}

template<typename Input, typename Next, typename F>
struct for_each_impl
    : range_action_closure_object<for_each_impl<Input, Next, F>,
                                  processing_style::incremental,
                                  processing_style::complete,
                                  Input,
                                  Next> {
  using output_type = void_tag&&;

  F f;

  constexpr void process_incremental(Input input) {
    f(std::forward<Input>(input));
  }

  constexpr decltype(auto) finish() {
    return this->next.process_complete(void_tag{});
  }
};

template<typename F>
constexpr auto for_each(F f) {
  return range_action_closure_factory<for_each_impl, F, F>{std::move(f)};
}

template<typename Input, typename Next>
struct values_impl
    : range_action_closure_object<values_impl<Input, Next>,
                                  processing_style::complete,
                                  processing_style::incremental,
                                  Input,
                                  Next> {
  using output_type = std::ranges::range_reference_t<std::remove_reference_t<Input>>;


  constexpr values_impl(Next&& next):
  range_action_closure_object<values_impl<Input, Next>,
                                  processing_style::complete,
                                  processing_style::incremental,
                                  Input,
                                  Next>(std::move(next)){}

  constexpr decltype(auto) process_complete(Input input) {
    for (auto&& v: input) {
      this->next.process_incremental(v);
    }
    return this->next.finish();
  }

};

constexpr auto values() {
  return range_action_closure_factory<values_impl, void>{};
}


template<typename Input, typename Next>
struct accumulate_in_place_impl
    : range_action_closure_object<accumulate_in_place_impl<Input, Next>,
                                  processing_style::incremental,
                                  processing_style::complete,
                                  Input,
                                  Next> {
  using output_type = int64_t;
  int64_t result = 0;

  constexpr decltype(auto) process_incremental(Input input) {
    result += input;
  }

  constexpr decltype(auto) finish(){
    return this->next.process_complete(std::move(result));
  }

};

constexpr auto sum(){
  return range::actions::range_action_closure_factory<accumulate_in_place_impl,int64_t>(0);
}

}
#include <vector>

constexpr int64_t calculate(){
  constexpr std::array v{1, 2, 3, 4};
  return range::actions::apply(v,
                               range::actions::values(),
                               range::actions::sum());
}

int main() {
  std::vector<int> v{1, 2, 3, 4};
  range::actions::apply(v,
                        range::actions::values(),
                        range::actions::for_each([](int i) {
                          std::cout << i << "\n";
                        }));
  static_assert(calculate() == 10);

  std::cout << "Hello, World!" << std::endl;
  return 0;
}
