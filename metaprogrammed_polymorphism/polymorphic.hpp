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

#include <array>
#include <cstdint>
#include <limits>
#include <memory>
#include <utility>

namespace polymorphic {
	namespace detail {

		// We have a lot of intermediate functions. We want to make sure that we forward
		// the parameters correctly. For non-reference parameters, we always move them.
		template <typename T> struct fwd_helper { using type = T&&; };
		template <typename T> struct fwd_helper<T&> { using type = T&; };
		template <typename T> struct fwd_helper<T&&> { using type = T&&; };

		template <typename T, typename U> decltype(auto) fwd(U&& u) {
			return static_cast<typename fwd_helper<T>::type>(u);
		}

		struct clone {};

		struct holder {
			void* ptr;
			virtual ~holder() {}
		};

		template <typename T> struct holder_imp : holder {
			T t_;
			holder_imp(T t) : t_(std::move(t)) { ptr = &t_; }
		};

		template <typename T> std::unique_ptr<holder> poly_extend(clone, const T& t) {
			return std::make_unique<holder_imp<T>>(t);
		}

		template <typename T> using ptr = T*;

		template <typename T> struct type {};

		template<typename T, typename Signature>
		struct trampoline;

		template<typename T, typename Return, typename Method, typename... Parameters>
		struct trampoline<T, Return(Method, Parameters...)> {
			static auto jump(void* t, Parameters... parameters) -> Return {
				return poly_extend(Method{},
					*static_cast<T*>(t),
					fwd<Parameters>(parameters)...);
			}
		};

		template<typename T, typename Return, typename Method, typename... Parameters>
		struct trampoline<T, Return(Method, Parameters...)const> {
			static auto jump(const void* t, Parameters... parameters) ->Return {
				return poly_extend(Method{},
					*static_cast<const T*>(t),
					fwd<Parameters>(parameters)...);
			}
		};


		using vtable_fun = ptr<void()>;

		template<typename T, typename... Signatures>
		inline const vtable_fun vtable_entries[] = { reinterpret_cast<vtable_fun>(trampoline<T,Signatures>::jump)... };


		template <size_t I, typename Signature> struct vtable_caller;

		template <size_t I, typename Method, typename Return, typename... Parameters>
		struct vtable_caller<I, Return(Method, Parameters...)> {

			static decltype(auto) call_imp(const vtable_fun* vt,
				Method,
				void* t, Parameters... parameters) {
				return reinterpret_cast<fun_ptr>(vt[I])(
					t, fwd<Parameters>(parameters)...);
			}

			static constexpr auto get_index(type<Return(Method, Parameters...)>) { return I; }


			using is_const = std::false_type;
			using fun_ptr = ptr<Return(void*, Parameters...)>;
		};


		template <size_t I, typename Method, typename Return, typename... Parameters>
		struct vtable_caller<I, Return(Method, Parameters...) const> {

			static decltype(auto) call_imp(const vtable_fun* vt,
				Method,
				const void* t, Parameters... parameters) {
				return reinterpret_cast<fun_ptr>(vt[I])(
					t, fwd<Parameters>(parameters)...);
			}

			static constexpr auto get_index(type<Return(Method, Parameters...)const>) { return I; }

			using is_const = std::true_type;
			using fun_ptr = ptr<Return(const void*, Parameters...)>;
		};

		template <typename Sequence, typename... Signatures> struct vtable_imp;

		template<typename OtherImp, typename... Signatures>
		static constexpr int get_offset() {
			constexpr std::array<int, sizeof...(Signatures)> ar{ static_cast<int>(OtherImp::get_index(type<Signatures>{}))... };
			auto first = ar[0];
			auto previous = first;
			for (int i = 1; i < ar.size(); ++i) {
				if (ar[i] != previous + 1) {
					return -1;
				}
				previous = ar[i];
			}
			return first;
		}



		template <size_t... I, typename... Signatures>
		struct vtable_imp<std::index_sequence<I...>, Signatures...>
			: vtable_caller<I, Signatures>... {
			using vtable_caller<I, Signatures>::call_imp...;
			using vtable_caller<I, Signatures>::get_index...;

			static constexpr bool all_const() {
				return std::conjunction_v<typename vtable_caller<I, Signatures>::is_const...>;
			}


			template <typename T>
			vtable_imp(type<T> t) : vptr_(&vtable_entries<T, Signatures...>[0]) {}

			const vtable_fun* vptr_;

			template <typename OtherSequence, typename... OtherSignatures>
			vtable_imp(const vtable_imp<OtherSequence, OtherSignatures...>& other)
				: vptr_(other.vptr_ + get_offset<vtable_imp<OtherSequence, OtherSignatures...>, Signatures...>())
			{
				static_assert(get_offset<vtable_imp<OtherSequence, OtherSignatures...>, Signatures...>() != -1);
			}

			vtable_imp(const vtable_imp&) = default;

			template <typename VoidType, typename Method, typename... Parameters>
			decltype(auto) call(Method method, VoidType t,
				Parameters&&... parameters) const {
				return vtable_imp::call_imp(vptr_, method, t,
					std::forward<Parameters>(parameters)...);
			}
		};






		template <typename... Signatures>
		using vtable =
			vtable_imp<std::make_index_sequence<sizeof...(Signatures)>, Signatures...>;


		template <typename T>
		struct is_polymorphic : std::false_type {};

		template<typename Sequence, typename... Signatures>
		class ref_impl;

		template<typename Sequence, typename... Signatures>
		class obj_impl;




		template<typename Sequence, typename... Signatures>
		struct is_polymorphic<ref_impl<Sequence, Signatures...>> :std::true_type {};

		template<typename Sequence, typename... Signatures>
		struct is_polymorphic<obj_impl<Sequence, Signatures...>> :std::true_type {};







		template <size_t... I, typename... Signatures>
		class ref_impl<std::index_sequence<I...>, Signatures...> :private vtable_caller<I, Signatures>... {

			const detail::vtable_fun* vptr_;
			std::conditional_t<std::conjunction_v<typename vtable_caller<I, Signatures>::is_const...>, const void*, void*> t_;

		public:
			template <typename T>
			ref_impl(T& t) : ref_impl(t, detail::is_polymorphic<std::decay_t<T>>{}) {}

			template<typename T>
			ref_impl(T& t, std::false_type) : vptr_(&detail::vtable_entries<std::decay_t<T>,Signatures...>[0]), t_(&t) {}

			template<typename Poly>
			ref_impl(Poly& other, std::true_type) : vptr_(other.vptr_ + get_offset<Poly,Signatures...>()), t_(other.get_ptr()) {
				static_assert(get_offset<Poly, Signatures...>() != -1);
			}

			explicit operator bool() const { return t_ != nullptr; }

			auto get_ptr() const { return t_; }

			template <typename Method, typename... Parameters>
			decltype(auto) call(Parameters&&... parameters) const {
				return this->call_imp(vptr_, Method{}, t_,
					std::forward<Parameters>(parameters)...);
			}
		};

	} // namespace detail

	template<typename... Signatures>
	using ref = detail::ref_impl < std::make_index_sequence<sizeof...(Signatures)>, Signatures...>;


	using copyable = std::unique_ptr<detail::holder>(detail::clone) const;

	namespace detail {

		template <bool all_const, typename... Signatures> class object_imp {
			using vtable_t = const detail::vtable<Signatures...>;

			vtable_t vt_;
			std::unique_ptr<holder> t_;

		public:
			const auto& get_vtable() const { return vt_; }
			template <typename T, typename = std::enable_if_t<
				!detail::is_polymorphic<std::decay_t<T>>::value>>
				object_imp(T t)
				: vt_(detail::type<T>{}),
				t_(std::make_unique<detail::holder_imp<T>>(std::move(t))) {}

			template <typename Poly, typename = std::enable_if_t<detail::is_polymorphic<
				std::decay_t<Poly>>::value>>
				object_imp(const Poly& other)
				: vt_(other.get_vtable()),
				t_(other ? other.template call<detail::clone>() : nullptr) {}

			object_imp(const object_imp& other)
				: vt_{ other.get_vtable() },
				t_(other ? other.call<detail::clone>() : nullptr) {}

			object_imp(object_imp&&) = default;

			object_imp& operator=(object_imp&&) = default;

			object_imp& operator=(const object_imp& other) { (*this) = ref<Signatures...>(other); }

			explicit operator bool() const { return t_ != nullptr; }

			void* get_ptr() { return t_ ? t_->ptr : nullptr; }
			const void* get_ptr() const { return t_ ? t_->ptr : nullptr; }

			template <typename Method, typename... Parameters>
			decltype(auto) call(Parameters&&... parameters) {
				return vt_.call(Method{}, get_ptr(),
					std::forward<Parameters>(parameters)...);
			}

			template <typename Method, typename... Parameters>
			decltype(auto) call(Parameters&&... parameters) const {
				return vt_.call(Method{}, get_ptr(),
					std::forward<Parameters>(parameters)...);
			}
		};

		template <typename... Signatures> class object_imp<true, Signatures...> {
			using vtable_t = const detail::vtable<Signatures...>;

			vtable_t vt_;
			std::shared_ptr<const holder> t_;

		public:
			const auto& get_vtable() const { return vt_; }
			template <typename T, typename = std::enable_if_t<
				!detail::is_polymorphic<std::decay_t<T>>::value>>
				object_imp(T t)
				: vt_(detail::type<T>{}),
				t_(std::make_shared<holder_imp<T>>(std::move(t))) {}

			template <typename Poly, typename = std::enable_if_t<detail::is_polymorphic<
				std::decay_t<Poly>>::value>>
				object_imp(const Poly& other)
				: vt_(other.get_vtable()),
				t_(other ? other.template call<detail::clone>() : nullptr) {}

			template <typename... OtherSignatures>
			object_imp(const object_imp<true, OtherSignatures...>& other)
				: vt_(other.get_vtable()), t_(other.t_) {}

			object_imp(const object_imp&) = default;

			object_imp(object_imp&&) = default;

			object_imp& operator=(object_imp&&) = default;

			object_imp& operator=(const object_imp&) = default;

			explicit operator bool() const { return t_ != nullptr; }

			const void* get_ptr() const { return t_ ? t_->ptr : nullptr; }

			template <typename Method, typename... Parameters>
			decltype(auto) call(Parameters&&... parameters) const {
				return vt_.call(Method{}, get_ptr(),
					std::forward<Parameters>(parameters)...);
			}
		};

		template<bool all_const,typename... Signatures>
		struct is_polymorphic<object_imp<all_const,Signatures...>> :std::true_type {};



	} // namespace detail

	template <typename... Signatures>
	using object = detail::object_imp<detail::vtable<Signatures...>::all_const(),
		Signatures..., copyable >;

} // namespace polymorphic
