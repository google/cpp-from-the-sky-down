// Copyright 2018 Google LLC
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     https://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// limitations under the License.

#pragma once
#include <type_traits>
namespace tafn {
	template <typename T>
	struct type {};

	struct all_types {};

	template <typename F>
	struct all_functions {};

	namespace detail {

		struct call_customization_point_imp_t {
			template <typename... Args, typename = std::void_t<decltype(tafn_customization_point(std::declval<Args>()...))>>
			decltype(auto) operator()(Args&&... args) const{
				return tafn_customization_point(std::forward<Args>(args)...);
			}
		};

		inline constexpr call_customization_point_imp_t call_customization_point_imp;

	}  // namespace detail

	template <bool b, typename F, typename T, typename... Args>
	struct has_customization_point {
		static constexpr bool value = false;
	};

	template <typename F, typename T, typename... Args>
	struct has_customization_point<true, F, T, Args...> {
		static constexpr bool value =
			std::is_invocable_v<detail::call_customization_point_imp_t, F, T,
			Args...>;
	};

	template <bool b, typename F, typename T, typename... Args>
	inline constexpr bool has_customization_point_v =
		has_customization_point<b, F, T, Args...>::value;

	enum class customization_point_type {
		type_function,
		all_function,
		type_all,
		all_all,
		none
	};

	// Calculate which customization point to use. Prefer type and function specific, though if have to choose, prefer function specific over type specific.
	template <typename F, typename T, typename... Args>
	struct get_customization_type {
		using D = std::decay_t<T>;
		static constexpr bool type_function =
			has_customization_point_v<true, F, type<D>, T, Args...>;
		static constexpr bool type_all = has_customization_point < !type_function,
			all_functions<F>, type<D>, T, Args... > ::value;
		static constexpr bool all_function =
			has_customization_point_v<!type_function && !type_all, F, all_types, T, Args...>;
		static constexpr bool all_all = has_customization_point_v < !type_function &&
			!all_function && !type_all,
			all_functions<F>, all_types, T, Args... >;

		static constexpr customization_point_type value =
			type_function
			? customization_point_type::type_function
			: all_function
			? customization_point_type::all_function
			: type_all ? customization_point_type::type_all
			: all_all ? customization_point_type::all_all
			: customization_point_type::none;
	};

	// For all_functions, make sure we do  not all_functions again as that will create infinite recursion.
	template <typename F, typename T, typename... Args>
	struct get_customization_type<all_functions<F>, T, Args...> {
		using D = std::decay_t<T>;
		static constexpr bool type_function =
			has_customization_point_v<true, F, type<D>, T, Args...>;
		static constexpr bool all_function =
			has_customization_point_v<!type_function, F, all_types, T, Args...>;
		static constexpr customization_point_type value =
			type_function
			? customization_point_type::type_function
			: all_function
			? customization_point_type::all_function
			: customization_point_type::none;
	};




	template <typename F, typename... Args>
	inline constexpr bool is_valid = get_customization_type<F, Args...>::value != customization_point_type::none;


	namespace detail {

		template <typename F>
		struct call_customization_point_t {
			template <typename T, typename... Args>
			decltype(auto) operator()(T&& t, Args&&... args) const {
				using D = std::decay_t<T>;

				constexpr auto customization_type =
					get_customization_type<F, T, Args...>::value;
				static_assert(customization_type != customization_point_type::none,
					"No implementation of F for T");
				if constexpr (customization_type ==
					customization_point_type::type_function) {
					return call_customization_point_imp(F{}, type<D>{}, std::forward<T>(t),
						std::forward<Args>(args)...);
				}
				if constexpr (customization_type == customization_point_type::type_all) {
					return call_customization_point_imp(all_functions<F>{}, type<D>{},
						std::forward<T>(t),
						std::forward<Args>(args)...);
				}

				if constexpr (customization_type ==
					customization_point_type::all_function) {
					return call_customization_point_imp(F{}, all_types{}, std::forward<T>(t),
						std::forward<Args>(args)...);
				}
				if constexpr (customization_type == customization_point_type::all_all) {
					return call_customization_point_imp(all_functions<F>{}, all_types{},
						std::forward<T>(t),
						std::forward<Args>(args)...);
				}
			}
		};
		template <typename F>
		struct call_customization_point_t<all_functions<F>> {
			template <typename T, typename... Args>
			decltype(auto) operator()(T&& t, Args&&... args) const {
				using D = std::decay_t<T>;

				constexpr auto customization_type =
					get_customization_type<all_functions<F>, T, Args...>::value;

				static_assert(
					customization_type == customization_point_type::type_function ||
					customization_type == customization_point_type::type_all,
					"No Implementation of F for T");
				if constexpr (customization_type ==
					customization_point_type::type_function) {
					return call_customization_point_imp(F{}, type<D>{}, std::forward<T>(t),
						std::forward<Args>(args)...);
				}
				if constexpr (customization_type == customization_point_type::type_all) {
					return call_customization_point_imp(F{}, all_types{}, std::forward<T>(t),
						std::forward<Args>(args)...);
				}
			}
		};

	}  // namespace detail

	template <typename F>
	inline constexpr detail::call_customization_point_t<F>
		call_customization_point{};

	namespace detail {
		template<class F, class Tuple>
		struct f_args_holder {
			Tuple  t;
		};

		template<typename F,size_t... I, typename T, typename Tuple>
		decltype(auto) call_with_tuple_imp(std::index_sequence<I...>, T&& t, Tuple&& tuple) {
			return call_customization_point<F>(std::forward<T>(t), std::get<I>(std::forward<Tuple>(tuple))...);
		}

		template<typename F,typename T, typename Tuple>
		decltype(auto) call_with_tuple(T&& t, Tuple&& tuple) {
			return call_with_tuple_imp<F>(std::make_index_sequence<std::tuple_size_v<std::decay_t<Tuple>>>(),std::forward<T>(t),std::forward<Tuple>(tuple));
		}



		template<typename F, typename... Args>
		auto make_f_args_holder(Args&&... args) {
			return f_args_holder < F,
				decltype(std::forward_as_tuple(std::forward<Args>(args)...)) > {std::forward_as_tuple(std::forward<Args>(args)...)};
		}

		template<typename T, typename F, typename Tuple>
		decltype(auto) operator|(T&& t, f_args_holder<F, Tuple>&& h) {
			return call_with_tuple<F>(std::forward<T>(t), std::move(h.t));
		}

		template<typename T, typename F, typename Tuple>
		decltype(auto) operator*(T&& t, f_args_holder<F, Tuple>&& h) {
			return call_with_tuple<F>(std::forward<T>(t), std::move(h.t));
		}

		template<typename F, typename Tuple>
		decltype(auto) operator*(f_args_holder<F, Tuple>&& h) {
			return std::apply(call_customization_point<F>,
				std::move(h.t));
		}




		template<typename F>
		struct f_args_maker {
			template<typename... Args>
			auto operator()(Args&&... args) const {
				return make_f_args_holder<F>(std::forward<Args>(args)...);
			}
		};


	}
	template<typename F>
	inline constexpr detail::f_args_maker<F> _{};











}  // namespace tafn
