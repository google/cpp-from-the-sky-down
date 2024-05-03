#include <iostream>
#include <ranges>
#include <type_traits>
#include <tuple>

namespace range::actions {

enum class processing_style {
  incremental,
  complete
};

template<typename aco, processing_style ips>
concept aco_input = aco::input_processing_style == ips;

template<typename aco, processing_style ops>
concept aco_output = aco::input_processing_style == ops;

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
template<processing_style ips, processing_style ops, typename Input, typename Next>
class range_action_closure_object {
  [[no_unique_address]] Next next_;

 public:
  using input_type = Input;

  constexpr static auto input_processing_style = ips;
  constexpr static auto output_processing_style = ops;

  range_action_closure_object(Next&& next) : next_(std::move(next)) {}

  constexpr Next& next() {
    return next_;
  }

  constexpr decltype(auto) finish()requires(incremental_input<
      range_action_closure_object>)
  {
    return next().finish();
  }

  constexpr bool done() const requires(incremental_input<
      range_action_closure_object>)
  {
    return next().done();
  }

  constexpr void process_incremental(input_type input)requires(incremental_input<
      range_action_closure_object>)
  {
    return next().process_incremental(static_cast<input_type>(input));
  }

  constexpr decltype(auto) process_complete(input_type input)requires(complete_output<
      range_action_closure_object>){
    return next().process_complete(static_cast<input_type>(input));

  }

};

struct void_tag {};
template<template<typename, typename, typename...> typename aco,
    typename parameters, typename... TypeParameters>
class range_action_closure_factory {
  [[no_unique_address]] parameters parameters_;

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
    return aco<Input, Next, TypeParameters...>{std::forward<Next>(next),
                                               std::move(parameters_)};
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
  constexpr auto make() {
    return previous.make(end_aco{});
  }
};

struct end_factory_tag {};

struct empty {
  template<typename Input, typename Next>
  constexpr auto make(Next&& next){
    return std::forward<Next>(next);
  }
};

template<typename Input>
struct starting_factory {
  template<typename>
  using input_type = Input;
  using output_type = Input;
  template<int&...,typename Next>
  constexpr auto make(Next&& next) {
    return std::forward<Next>(next);
  }
    template<typename,typename Next>
  constexpr auto make(Next&& next) {
    return std::forward<Next>(next);
  }
};
template<typename Output, typename Factory = empty, typename Previous = empty>
struct input_factory {
  using output_type = Output;
  using previous_t = Previous;
  Factory& factory;
  Previous& previous;
  template<typename NewFactory>
  constexpr auto operator+(NewFactory&& new_factory) {
    return input_factory<typename Factory::template input_type<Output>,
                         NewFactory,
                         input_factory>
        {new_factory, *this};
  }
  constexpr auto operator+(end_factory_tag) {
    return end_factory<input_factory>{*this};
  }

  template<typename Next>
  constexpr auto make(Next&& next) {
    return previous.make(factory.template make<typename Previous::output_type>(
        std::forward<Next>(next)));
  }

};
}

template<typename Input, typename Next, typename F>
struct for_each_impl
    : range_action_closure_object<processing_style::incremental,
                                  processing_style::complete, Input, Next> {
  using output_type = void_tag&&;

  F f;

  constexpr for_each_impl(Next&& n, F f) : range_action_closure_object<
      processing_style::incremental,
      processing_style::complete,
      Input,
      Next>(std::move(n)), f(std::move(f)) {}

  constexpr void process_incremental(Input input) {
    f(std::forward<Input>(input));
  }

  constexpr decltype(auto) finish() {
    return this->next().process_complete(void_tag{});
  }
};

template<typename F>
auto for_each(F f) {
  return range_action_closure_factory<for_each_impl, F, F>{std::move(f)};
}

template<typename Range, typename... Acos>
auto apply(Range&& range, Acos&& ... acos) {

  detail::empty empty;
  detail::starting_factory<decltype(range)> starting_factory;
  auto chain = (detail::input_factory<decltype(range),
                                      detail::starting_factory<decltype(range)>,
                                          detail::starting_factory<decltype(range)>>{
      starting_factory, starting_factory} + ... + std::forward<
      Acos>(acos)).make(detail::end_aco{});
  return chain.process_complete(std::forward<Range>(range));
}

}
#include <vector>

int main() {
  std::vector<int> v{1, 2, 3, 4};
  range::actions::apply(v, range::actions::for_each([](int i) {
    std::cout << i << "\n";
  }));

  std::cout << "Hello, World!" << std::endl;
  return 0;
}
