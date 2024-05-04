#include <iostream>
#include <ranges>
#include <type_traits>
#include <tuple>

namespace ranges::actions {

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

template<typename Input, typename Next, processing_style PreviousOutputProcessingStyle,
    processing_style InputProcessingStyle, processing_style OutputProcessingStyle>
struct opaque {
  using input_type = Input;
  using next_type = Next;
  static constexpr auto previous_output_processing_style = PreviousOutputProcessingStyle;
  static constexpr auto input_processing_style = InputProcessingStyle;
  static constexpr auto output_processing_style = OutputProcessingStyle;
};

// Handle complete -> incremental conversions
template<typename Input, processing_style PreviousOutputProcessingStyle, processing_style InputProcessingStyle>
struct adapt_input;

template<typename Input, processing_style PreviousOutputProcessingStyle, processing_style InputProcessingStyle> requires (
    PreviousOutputProcessingStyle == InputProcessingStyle)
struct adapt_input<Input,PreviousOutputProcessingStyle,InputProcessingStyle> {
  using type = Input;
};

template<typename Input>
struct adapt_input<Input,
                   processing_style::complete,
                   processing_style::incremental> {
  using type = std::ranges::range_reference_t<std::remove_reference_t<Input>>;
};

template<typename Input, processing_style PreviousOutputProcessingStyle, processing_style InputProcessingStyle>
using adapt_input_t = typename adapt_input<Input,
                                           PreviousOutputProcessingStyle,
                                           InputProcessingStyle>::type;

template<typename Child>
struct range_action_closure_object;

template<template<typename...> typename Derived, typename Opaque, typename... Parameters>
struct range_action_closure_object<Derived<Opaque, Parameters...>> {

  using child = Derived<Opaque, Parameters...>;
  using next_type = typename Opaque::next_type;
  [[no_unique_address]] next_type next;

  using raw_input_type = typename Opaque::input_type;
  using base = range_action_closure_object;

  constexpr static auto previous_output_processing_style =
      Opaque::previous_output_processing_style;
  constexpr static auto input_processing_style = Opaque::input_processing_style;
  constexpr static auto
      output_processing_style = Opaque::output_processing_style;

  using input_type = adapt_input_t<raw_input_type,
                                   previous_output_processing_style,
                                   input_processing_style>;

  constexpr range_action_closure_object(next_type&& next)
      : next(std::move(next)) {}

  constexpr decltype(auto) finish()requires(incremental_input<
      child>)
  {
    return next.finish();
  }

  constexpr bool done() const requires(incremental_input<
      child>)
  {
    return next.done();
  }

  constexpr void process_incremental(input_type input)requires(incremental_input<
      child>)
  {
    return next.process_incremental(static_cast<input_type>(input));
  }

  constexpr decltype(auto) process_complete(input_type input)requires(complete_input<
      child>){
    return next.process_complete(static_cast<input_type>(input));

  }

  constexpr decltype(auto) process_complete(raw_input_type input)requires (
      previous_output_processing_style == processing_style::complete
          && incremental_input<
              child>) {
    for (auto&& v: static_cast<raw_input_type>(input)) {
      static_cast<child&>(*this).process_incremental(std::forward<decltype(v)>(v));
    }

    static_cast<child&>(*this).finish();

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

template<template<typename, typename...> typename Aco, processing_style InputProcessingStyle,
    processing_style OutputProcessingStyle,
    typename Parameters = void, typename... TypeParameters>
class range_action_closure_factory {
  [[no_unique_address]] instantiable_t<Parameters> parameters_;

 public:
  static constexpr auto input_processing_style = InputProcessingStyle;
  static constexpr auto output_processing_style = OutputProcessingStyle;
  template<typename Input>
  using output_type = typename Aco<opaque<Input,
                                   void_tag,input_processing_style,
                                   input_processing_style,output_processing_style>,
                                   TypeParameters...>::output_type;
  template<typename... Ts>
  constexpr explicit range_action_closure_factory(Ts&& ... ts)
      :parameters_{std::forward<Ts>(ts)...} {}

  template<typename Opaque, typename Next>
  constexpr auto make(Next&& next) {
    if constexpr (std::is_same_v<Parameters, void>) {
      return Aco<Opaque, TypeParameters...>{std::forward<Next>(next)};
    } else {
      return Aco<Opaque, TypeParameters...>{std::forward<Next>(next),
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

struct starting_previous {
  template<typename Next>
  constexpr auto make(Next&& next) {
    return std::forward<Next>(next);
  }
};

template<typename Input>
struct starting_factory {
  template<typename>
  using output_type = Input;
  static constexpr auto input_processing_style = processing_style::complete;
  static constexpr auto output_processing_style = processing_style::complete;

  template<typename, typename Next>
  constexpr auto make(Next&& next) {
    return std::forward<Next>(next);
  }
};
template<typename Input, processing_style PreviousOutputProcessingStyle, typename Factory = starting_factory<Input>, typename Previous = starting_previous>
struct input_factory {
  static constexpr auto previous_output_processing_style = PreviousOutputProcessingStyle;
  using output_type = typename Factory::template output_type<Input>;
  static constexpr auto output_processing_style = Factory::output_processing_style;
  static constexpr auto input_processing_style = Factory::input_processing_style;
  Factory& factory;
  Previous& previous;
  template<typename NewFactory>
  constexpr auto operator+(NewFactory&& new_factory) {
    return input_factory<output_type, output_processing_style,
                         NewFactory,
                         input_factory>
        {new_factory, *this};
  }
  constexpr auto operator+(end_factory_tag) {
    return end_factory<input_factory>{*this};
  }

  template<typename Next>
  constexpr auto make(Next&& next) {
    return previous.make(factory.template make<opaque<Input,Next,previous_output_processing_style,
        input_processing_style,output_processing_style>>(
        std::forward<Next>(next)));
  }

};
}

template<typename Range, typename... Acos>
constexpr auto apply(Range&& range, Acos&& ... acos) {

  detail::starting_previous empty;
  detail::starting_factory<decltype(range)> starting_factory;
  auto chain = (detail::input_factory<decltype(range),processing_style::complete>{
      starting_factory, empty} + ... + std::forward<
      Acos>(acos)).make(detail::end_aco{});
  return chain.process_complete(std::forward<Range>(range));
}

template<typename Opaque, typename F>
struct for_each_impl
    : range_action_closure_object<for_each_impl<Opaque, F>> {
  using base = range_action_closure_object<for_each_impl>;
  using typename base::input_type;
  using typename base::next_type;
  using output_type = void_tag&&;

  F f;

  constexpr void process_incremental(input_type input) {
    f(std::forward<input_type>(input));
  }

  constexpr decltype(auto) finish() {
    return this->next.process_complete(void_tag{});
  }
};

template<typename F>
constexpr auto for_each(F f) {
  return range_action_closure_factory<for_each_impl,
                                      processing_style::incremental,
                                      processing_style::complete,
                                      F,
                                      F>{std::move(f)};
}

template<typename Opaque>
struct values_impl
    : range_action_closure_object<values_impl<Opaque>> {
  using base = range_action_closure_object<values_impl>;
  using typename base::input_type;
  using output_type = std::ranges::range_reference_t<std::remove_reference_t<
      input_type>>;

  template<typename Next>
  constexpr values_impl(Next&& next):
      base(std::move(next)) {}

  constexpr decltype(auto) process_complete(input_type input) {
    for (auto&& v: input) {
      this->next.process_incremental(v);
    }
    return this->next.finish();
  }

};

constexpr auto values() {
  return range_action_closure_factory<values_impl,
                                      processing_style::complete,
                                      processing_style::incremental,
                                      void>{};
}

template<typename Opaque>
struct accumulate_in_place_impl
    : range_action_closure_object<accumulate_in_place_impl<Opaque>
                                  > {
  using base = range_action_closure_object<accumulate_in_place_impl>;
  using typename base::input_type;
  using output_type = int64_t;
  int64_t result = 0;

  constexpr decltype(auto) process_incremental(input_type input) {
    result += input;
  }

  constexpr decltype(auto) finish() {
    return this->next.process_complete(std::move(result));
  }

};

constexpr auto sum() {
  return ranges::actions::range_action_closure_factory<accumulate_in_place_impl,
                                                       processing_style::incremental, processing_style::complete,
                                                       int64_t>(0);
}

}
#include <vector>

constexpr int64_t calculate() {
  constexpr std::array v{1, 2, 3, 4};
  return ranges::actions::apply(v,
                                ranges::actions::values(),
                                ranges::actions::sum());
}

int main() {
  std::vector<int> v{1, 2, 3, 4};
  ranges::actions::apply(v,
                         ranges::actions::for_each([](int i) {
                          std::cout << i << "\n";
                        }));
  static_assert(calculate() == 10);

  std::cout << "Hello, World!" << std::endl;
  return 0;
}
