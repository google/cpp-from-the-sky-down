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

		class value_holder {
			struct holder_interface {
				virtual std::unique_ptr<holder_interface> clone() = 0;
				virtual ~holder_interface() {}
				void* ptr_ = nullptr;
			};
			template<typename T>
			struct holder_impl :holder_interface {
				holder_impl(T t) :t_(std::move(t)) { ptr_ = &t_; }
				std::unique_ptr<holder_interface> clone() override {
					return std::make_unique<holder_impl<T>>(t_);
				}
				T t_;
			};
			std::unique_ptr<holder_interface> impl_;
		public:
			template<typename T>
			value_holder(T t) :impl_(std::make_unique<holder_impl<T>>(std::move(t))) {}

			value_holder(value_holder&&) = default;
			value_holder& operator=(value_holder&&) = default;

			value_holder(const value_holder& other) :impl_(other.impl_ ? other.impl_->clone() : nullptr) {}
			value_holder& operator=(const value_holder& other) {
				return (*this) = value_holder(other);
			}
			void* get_ptr() { return impl_ ? impl_->ptr_ : nullptr; }
			const void* get_ptr()const { return impl_ ? impl_->ptr_ : nullptr; }
		};

		template<typename T>
		struct ptr_holder {
			T* ptr_;
			T* get_ptr()const { return ptr_; }
			template<typename V>
			ptr_holder(V& v) :ptr_(&v) {}
			ptr_holder(value_holder& v) :ptr_(v.get_ptr()) {}
			ptr_holder(const value_holder& v) :ptr_(v.get_ptr()) {}
		};

		template <typename T> using ptr = T*;
		template <typename T> struct type {};
		template <typename... F> struct overload : F... { using F::operator()...; };

		template <typename T, typename Signature> struct trampoline;

		template <typename T, typename Return, typename Method, typename... Parameters>
		struct trampoline<T, Return(Method, Parameters...)> {
			static auto jump(void* t, Parameters... parameters) -> Return {
				return poly_extend(Method{}, *static_cast<T*>(t),
					fwd<Parameters>(parameters)...);
			}
		};

		template <typename T, typename Return, typename Method, typename... Parameters>
		struct trampoline<T, Return(Method, Parameters...) const> {
			static auto jump(const void* t, Parameters... parameters) -> Return {
				return poly_extend(Method{}, *static_cast<const T*>(t),
					fwd<Parameters>(parameters)...);
			}
		};

		using vtable_fun = ptr<void()>;

		template <typename T, typename... Signatures>
		inline const vtable_fun vtable[] = {
			reinterpret_cast<vtable_fun>(trampoline<T, Signatures>::jump)... };

		template <size_t I, typename Signature> struct vtable_caller;

		template <size_t I, typename Method, typename Return, typename... Parameters>
		struct vtable_caller<I, Return(Method, Parameters...)> {
			decltype(auto) operator()(const vtable_fun* vt, Method, void* t,
				Parameters... parameters) const {
				return reinterpret_cast<ptr<Return(void*, Parameters...)>>(vt[I])(
					t, fwd<Parameters>(parameters)...);
			}
		};

		template <std::size_t I, typename Method, typename Return, typename... Parameters>
		struct vtable_caller<I, Return(Method, Parameters...) const> {
			decltype(auto) operator()(const vtable_fun* vt, Method, const void* t,
				Parameters... parameters) const {
				return reinterpret_cast<ptr<Return(const void*, Parameters...)>>(vt[I])(t, fwd<Parameters>(parameters)...);
			}
		};

		template <typename Signature> struct is_const_signature : std::false_type {};

		template <typename Method, typename Return, typename... Parameters>
		struct is_const_signature<Return(Method, Parameters...) const>
			: std::true_type {};

		template <std::size_t I, typename Signature> struct index_getter {
			constexpr int operator()(type<Signature>) const { return I; }
		};


		template <typename Holder, typename Sequence, typename... Signatures>
		class ref_impl;

		template <typename Holder, size_t... I, typename... Signatures>
		class ref_impl<Holder, std::index_sequence<I...>, Signatures...>
			: private vtable_caller<I, Signatures>... {

			template <typename OtherHolder, typename OtherSequence, typename... OtherSignatures>
			friend class ref_impl;

			const detail::vtable_fun* vptr_;
			Holder t_;

			static constexpr overload<vtable_caller<I, Signatures>...> call_vtable{};
			static constexpr overload<index_getter<I, Signatures>...> get_index{};

			template <typename Other> static constexpr int get_offset() {
				constexpr std::array<int, sizeof...(Signatures)> ar{
					static_cast<int>(Other::get_index(type<Signatures>{}))... };
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

			struct other_ref {};

			template <typename T>
			ref_impl(T&& other, other_ref)
				: vptr_(other.vptr_ + get_offset<std::decay_t<T>>()),
				t_(std::forward<T>(other).t_) {
				static_assert(get_offset<std::decay_t<T>>() != -1);
			}

		public:
			template <typename T>
			ref_impl(T&& t)
				: vptr_(&detail::vtable<std::decay_t<T>, Signatures...>[0]),
				t_(std::forward<T>(t)) {}



			template <typename OtherHolder, typename OtherSequence,
				typename... OtherSignatures>
				ref_impl(
					const ref_impl<OtherHolder, OtherSequence, OtherSignatures...>& other)
				: ref_impl(other, other_ref{}) {}

			template <typename OtherHolder, typename OtherSequence,
				typename... OtherSignatures>
				ref_impl(
					ref_impl<OtherHolder, OtherSequence, OtherSignatures...>& other)
				: ref_impl(other, other_ref{}) {}



			template <typename OtherHolder, typename OtherSequence,
				typename... OtherSignatures>
				ref_impl(ref_impl<OtherHolder, OtherSequence, OtherSignatures...>&& other)
				: ref_impl(std::move(other), other_ref{}) {}

			explicit operator bool() const { return t_ != nullptr; }

			auto get_ptr() const { return t_; }

			template <typename Method, typename... Parameters>
			decltype(auto) call(Parameters&&... parameters) const {
				return call_vtable(vptr_, Method{}, t_.get_ptr(),
					std::forward<Parameters>(parameters)...);
			}

			template <typename Method, typename... Parameters>
			decltype(auto) call(Parameters&&... parameters) {
				return call_vtable(vptr_, Method{}, t_.get_ptr(),
					std::forward<Parameters>(parameters)...);
			}

		};



	} // namespace detail

	template <typename... Signatures>
	using ref = detail::ref_impl<
		detail::ptr_holder<std::conditional_t<
		std::conjunction_v<detail::is_const_signature<Signatures>...>,
		const void, void>>,
		std::make_index_sequence<sizeof...(Signatures)>, Signatures...>;

	template <typename... Signatures>
	using object = detail::ref_impl<
		detail::value_holder,
		std::make_index_sequence<sizeof...(Signatures)>, Signatures...>;





} // namespace polymorphic
