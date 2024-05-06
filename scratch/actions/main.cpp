#include <iostream>
#include <ranges>
#include <type_traits>
#include <functional>
#include <tuple>

namespace ranges::actions {

enum class processing_style {
  incremental,
  complete
};

template<typename SizeType = std::monostate, bool Exact = false>
struct propagated_size{
  constexpr bool has_size() const {return !std::same_as<SizeType, std::monostate>;}
  constexpr auto size() const{return size_;}
  constexpr bool is_exact_size() const {return Exact;}
  [[no_unique_address]] SizeType size_;
  explicit constexpr propagated_size()
  requires(std::same_as<SizeType,std::monostate>)
  {}

  explicit constexpr propagated_size(size_t s)
  requires(std::same_as<SizeType, size_t>)
  :size_(s){}

  constexpr auto min(size_t new_value)
  requires (has_size())
  {
    return propagated_size(std::min(size(),new_value));
  }

  constexpr auto min(size_t new_value)
  requires (!has_size())
  {
    return propagated_size<size_t, false>(new_value);
  }

  constexpr auto operator+(size_t new_value)
  requires(has_size()){
    return propagated_size(size() + new_value);
  }
  constexpr auto operator+(size_t new_value)
  requires(!has_size()){
    return propagated_size<size_t, false>(new_value);
  }

  constexpr auto operator*(size_t new_value)
  requires(has_size()){
    return propagated_size(size() * new_value);
  }

  constexpr auto operator*(size_t new_value)
  requires(!has_size()){
    return propagated_size<std::monostate,false>();
  }

  constexpr auto operator/(size_t new_value)
  requires(has_size()){
    return propagated_size(size() * new_value);
  }

  constexpr auto operator/(size_t new_value)
  requires(!has_size()){
    return propagated_size<std::monostate,false>();
  }
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

  // Utilities
  constexpr input_type forward(auto&& input){
    return static_cast<input_type>(input);
  }

};

template<typename T>
struct instantiable {
  using type = T;
};

template<>
struct instantiable<void> {
  using type = std::monostate;
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
                                   std::monostate,input_processing_style,
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
[[nodiscard]] constexpr auto apply(Range&& range, Acos&& ... acos) {

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
  using typename range_action_closure_object<for_each_impl>::base;
  using typename base::input_type;
  using typename base::next_type;
  using output_type = std::monostate&&;

  F f;

  constexpr void process_incremental(input_type input) {
    f(std::forward<input_type>(input));
  }

  constexpr decltype(auto) finish() {
    return this->next.process_complete(std::monostate{});
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

template<template<typename...> typename T>
struct template_to_typename{
};

template<typename T, typename>
struct apply_input_to_typename{
  using type = T;
};

template<template<typename...> typename T, typename Input>
struct apply_input_to_typename<template_to_typename<T>,Input>{
  using type = T<Input>;
};

template<typename T, typename Input>
using apply_input_to_typename_t = typename apply_input_to_typename<T,Input>::type;

template<typename Opaque, typename T, typename F>
struct accumulate_in_place_impl
    : range_action_closure_object<accumulate_in_place_impl<Opaque,T,F>
                                  > {
  using base = range_action_closure_object<accumulate_in_place_impl>;
  using typename base::input_type;
  using accumulated_type = std::remove_cvref_t<apply_input_to_typename_t<T,input_type>>;
  using output_type = accumulated_type&&;
  [[no_unique_address]] accumulated_type accumulated{};
  [[no_unique_address]] F f{};

  constexpr accumulate_in_place_impl(auto&& opaque, std::tuple<accumulated_type,F>&& tuple)
  :base(std::forward<decltype(opaque)>(opaque)),accumulated(std::get<0>(tuple)),
  f(std::get<1>(tuple)){}

  constexpr accumulate_in_place_impl(auto&& opaque, F&& f)
  :base(std::forward<decltype(opaque)>(opaque)),accumulated{},
  f(std::move(f)){}


  constexpr decltype(auto) process_incremental(input_type input) {
    std::invoke(f,accumulated, this->forward(input));
  }

  constexpr decltype(auto) finish() {
    return this->next.process_complete(std::move(accumulated));
  }

};

template<typename T, typename F>
constexpr auto accumulate_in_place(T t, F f){
  return range_action_closure_factory<accumulate_in_place_impl,processing_style::incremental,
  processing_style::complete,std::tuple<T,F>,T,F>(std::make_tuple(std::move(t), std::move(f)));
}

template<template<typename> typename T, typename F>
constexpr auto accumulate_in_place(F f){
  return range_action_closure_factory<accumulate_in_place_impl,processing_style::incremental,
  processing_style::complete,F,template_to_typename<T> ,F>(std::move(f));
}

template<typename T, typename F>
constexpr auto accumulate(T t, F f){
  return accumulate_in_place(std::forward<T>(t),[f = std::move(f)](auto& accumulated, auto&& v){
    accumulated = std::invoke(f,std::forward<decltype(v)>(v));
  });
}

template<template<typename> typename T, typename F>
constexpr auto accumulate(F f){
  return accumulate_in_place<T>([f = std::move(f)](auto& accumulated, auto&& v){
    accumulated = std::invoke(f,accumulated, std::forward<decltype(v)>(v));
  });
}

constexpr auto sum() {
  return accumulate<std::type_identity_t>(std::plus<>{});
}

}
#include <vector>

constexpr auto calculate() {
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
  static_assert(std::same_as<decltype(calculate()),int>);

  std::cout << "Hello, World!" << std::endl;
  return 0;
}
