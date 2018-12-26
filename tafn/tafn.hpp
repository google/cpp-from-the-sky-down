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
	template<typename T>
	struct type {};

	struct all_types {};

	namespace detail {

		struct call_customization_point_imp_t {
			template<typename... Args>
			auto operator()(Args&&... args) const -> decltype(customization_point(std::forward<Args>(args)...)) {
				return customization_point(std::forward<Args>(args)...);
			}
		};

		inline constexpr call_customization_point_imp_t call_customization_point_imp;

	}

	template<typename F, typename T, typename... Args>
	inline constexpr bool has_implementation = std::is_invocable_v<detail::call_customization_point_imp_t, F, type<T>, Args...> || std::is_invocable_v<detail::call_customization_point_imp_t, F, all_types, Args...>;

	template<typename F, typename T, typename... Args>
	inline constexpr bool has_type_specific_implementation = std::is_invocable_v<detail::call_customization_point_imp_t, F, type<T>, Args...>;



	namespace detail {
		template<typename F, typename T>
		struct call_customization_point_t {
			template<typename... Args>
			decltype(auto) operator()(Args&&... args) const {

				static_assert(has_implementation<F, T, Args...>, "no implementation for F for T");
				if constexpr (has_type_specific_implementation<F, T, Args...>) {
					return call_customization_point_imp(F{}, type<T>{}, std::forward<Args>(args)...);
				}
				else {
					return call_customization_point_imp(F{}, all_types{}, std::forward<Args>(args)...);
				}
			}
		};

	}

	template<typename F, typename T>
	inline constexpr detail::call_customization_point_t<F, T> call_customization_point{};



	namespace detail {

		template<typename T>
		struct lvalue_wrapper;

		template<typename T>
		struct rvalue_wrapper;



		template<typename F, typename T>
		struct call_and_wrap_customization_point_t {

			template<typename... Args>
			static auto helper(type<void>, Args&&... args) {
				return call_customization_point<F, T>(std::forward<Args>(args)...);
			}
			template<typename V, typename... Args>
			static auto helper(type<V>, Args&&... args) {
				if constexpr (std::is_rvalue_reference_v<V>) {
					return rvalue_wrapper<V>{call_customization_point<F, T>(std::forward<Args>(args)...)};
				}
				else {
					return lvalue_wrapper<V>{call_customization_point<F, T>(std::forward<Args>(args)...)};
				}
			}

			template<typename... Args>
			auto operator()(Args&&... args) const {
				using e_type = decltype(call_customization_point<F, T>(std::forward<Args>(args)...));
				return helper(type<e_type>{}, std::forward<Args>(args)...);
			}
		};



	}



	template<typename F, typename T>
	inline constexpr detail::call_and_wrap_customization_point_t<F, T> call_and_wrap_customization_point{};






	namespace detail {

		template<typename T>
		struct lvalue_wrapper {

			T unwrapped;
			using D = std::decay_t<T>;
			template<typename F, typename... Args>
			auto _(Args&&... args)  {
				return call_and_wrap_customization_point<F, D>(unwrapped, std::forward<Args>(args)...);
			}

			lvalue_wrapper(const lvalue_wrapper&) = delete;
			lvalue_wrapper& operator=(const lvalue_wrapper&) = delete;
		};

		template<typename T>
		struct rvalue_wrapper {

			T unwrapped;
			using D = std::decay_t<T>;
			template<typename F, typename... Args>
			auto _(Args&&... args) {
				return call_and_wrap_customization_point<F, D>(std::forward<T>(unwrapped), std::forward<Args>(args)...);
			}

			rvalue_wrapper(const rvalue_wrapper&) = delete;
			rvalue_wrapper& operator=(const rvalue_wrapper&) = delete;

		};

	}



	template<typename T>
	auto wrap(T&& t) {
		if constexpr (std::is_rvalue_reference_v<T>) {
			return detail::rvalue_wrapper<T&>{t};
		}
		else {
			return detail::lvalue_wrapper<T>{t};
		}
	}



	struct value {};

	template<typename T>
	decltype(auto) customization_point(value, all_types, T&& t) {
		return std::forward<T>(t);
	}

}
