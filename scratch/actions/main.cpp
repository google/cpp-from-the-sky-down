#include <iostream>
#include <ranges>
#include <type_traits>

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

template<typename Child, processing_style ips, processing_style ops,
    typename Input, typename Next>
class range_action_object {
  [[no_unique_address]] Next next_;

 public:
  constexpr static auto input_processing_style = ips;
  constexpr static auto output_processing_style = ops;

  constexpr Next& next() {
    return next_;
  }

  constexpr decltype(auto) finish()requires(incremental_input<
      range_action_object>)
  {
    return next().finish();
  }

  constexpr bool done() const requires(incremental_input<range_action_object>)
  {
    return next().done();
  }

  constexpr void process_incremental(Input input)requires(incremental_input<
      range_action_object>)
  {
    return next().process_incremental(static_cast<Input>(input));
  }

  constexpr decltype(auto) process_complete(Input input)requires(complete_output<
      range_action_object>){
    return process_complete(static_cast<Input>(input));

  }

};

}

int main() {
  std::cout << "Hello, World!" << std::endl;
  return 0;
}
