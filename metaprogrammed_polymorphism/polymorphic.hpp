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
			decltype(auto) operator()(const vtable_fun* vt, const std::uint8_t* permutation, Method, void* t,
				Parameters... parameters) const {
				return reinterpret_cast<ptr<Return(void*, Parameters...)>>(vt[permutation[I]])(
					t, fwd<Parameters>(parameters)...);
			}
		};

		template <std::size_t I, typename Method, typename Return, typename... Parameters>
		struct vtable_caller<I, Return(Method, Parameters...) const> {
			decltype(auto) operator()(const vtable_fun* vt, const std::uint8_t* permutation, Method, const void* t,
				Parameters... parameters) const {
				return reinterpret_cast<ptr<Return(const void*, Parameters...)>>(vt[permutation[I]])(t, fwd<Parameters>(parameters)...);
			}
		};

		template <typename Signature> struct is_const_signature : std::false_type {};

		template <typename Method, typename Return, typename... Parameters>
		struct is_const_signature<Return(Method, Parameters...) const>
			: std::true_type {};

		template <std::size_t I, typename Signature> struct index_getter {
			constexpr int operator()(type<Signature>) const { return I; }
		};

		struct value_tag {};

		template <typename Holder, typename Sequence, typename... Signatures>
		class ref_impl;

		template<typename T>
		struct is_ref_impl :std::false_type {};

		template <typename Holder, typename Sequence, typename... Signatures>
		struct is_ref_impl<ref_impl<Holder, Sequence, Signatures...>> :std::true_type {};

		template <typename Holder, size_t... I, typename... Signatures>
		class ref_impl<Holder, std::index_sequence<I...>, Signatures...> { 

			template <typename OtherHolder, typename OtherSequence, typename... OtherSignatures>
			friend class ref_impl;

			const detail::vtable_fun* vptr_;
			std::array<std::uint8_t, sizeof...(Signatures)> permutation_;
			Holder t_;

			static constexpr overload<vtable_caller<I, Signatures>...> call_vtable{};
			static constexpr overload<index_getter<I, Signatures>...> get_index{};

			template <typename T>
			ref_impl(T&& t, std::false_type)
				: vptr_(&detail::vtable<std::decay_t<T>, Signatures...>[0]), permutation_{ I... },
				t_(std::forward<T>(t), value_tag{}) {}

			template <typename OtherRef>
			ref_impl(OtherRef&& other, std::true_type)
				: vptr_(other.vptr_), permutation_{ other.permutation_[other.get_index(type<Signatures>{})]... },
				t_(std::forward<OtherRef>(other).t_) {
			}

		public:
			template <typename T>
			ref_impl(T&& t) :ref_impl(std::forward<T>(t), is_ref_impl<std::decay_t<T>>{}) {}

			explicit operator bool() const { return t_ != nullptr; }

			auto get_ptr() const { return t_.get_ptr(); }
			auto get_ptr() { return t_.get_ptr(); }

			template <typename Method, typename... Parameters>
			decltype(auto) call(Parameters&&... parameters) const {
				return call_vtable(vptr_, permutation_.data(), Method{}, t_.get_ptr(),
					std::forward<Parameters>(parameters)...);
			}

			template <typename Method, typename... Parameters>
			decltype(auto) call(Parameters&&... parameters) {
				return call_vtable(vptr_, permutation_.data(), Method{}, t_.get_ptr(),
					std::forward<Parameters>(parameters)...);
			}
		};

		struct holder_interface {
			virtual std::unique_ptr<holder_interface> clone()const = 0;
			virtual ~holder_interface() {}
			void* ptr_ = nullptr;
		};
		template<typename T>
		struct holder_impl :holder_interface {
			holder_impl(T t) :t_(std::move(t)) { ptr_ = &t_; }
			std::unique_ptr<holder_interface> clone() const override {
				return std::make_unique<holder_impl<T>>(t_);
			}
			T t_;
		};

		class value_holder {
			std::unique_ptr<holder_interface> impl_;
			void* ptr_ = nullptr;
		public:
			template<typename T>
			value_holder(T t, value_tag) :impl_(std::make_unique<holder_impl<T>>(std::move(t))), ptr_(get_ptr_impl()) {}

			value_holder(value_holder&& other)noexcept :impl_(std::move(other.impl_)), ptr_(other.ptr_) {
				other.ptr_ = nullptr;
			}
			value_holder& operator=(value_holder&& other)noexcept {
				impl_ = std::move(other.impl_);
				ptr_ = std::move(other.ptr_);
				other.ptr_ = nullptr;
				return *this;
			}

			value_holder(const value_holder& other) :impl_(other.impl_ ? other.impl_->clone() : nullptr), ptr_(get_ptr_impl()) {}
			value_holder& operator=(const value_holder& other) {
				return (*this) = value_holder(other);
			}
			void* get_ptr_impl() { return impl_ ? impl_->ptr_ : nullptr; }
			void* get_ptr() { return ptr_; }
			const void* get_ptr()const { return ptr_; }

			auto clone_ptr()const { return impl_ ? impl_->clone() : nullptr; }
		};

		template<typename T>
		struct ptr_holder {
			T* ptr_;
			T* get_ptr()const { return ptr_; }
			template<typename V>
			ptr_holder(V& v, value_tag) :ptr_(&v) {}

			template<typename OtherT>
			ptr_holder(const ptr_holder<OtherT>& other) : ptr_(other.get_ptr()) {}
			template<typename OtherT>
			ptr_holder& operator=(const ptr_holder<OtherT>& other) {
				return (*this) = ptr_holder(other);
			}

			ptr_holder(value_holder& v) :ptr_(v.get_ptr()) {}
			ptr_holder(const value_holder& v) :ptr_(v.get_ptr()) {}
		};

		struct shared_ptr_holder {
			std::shared_ptr<const holder_interface> impl_;
			const void* ptr_ = nullptr;
			const void* get_ptr()const { return ptr_; }
			const void* get_ptr_impl()const { return impl_ ? impl_->ptr_ : nullptr; }
			shared_ptr_holder(const shared_ptr_holder&) = default;
			shared_ptr_holder& operator=(const shared_ptr_holder&) = default;
			shared_ptr_holder(shared_ptr_holder&& other)noexcept :impl_(std::move(other.impl_)), ptr_(other.ptr_) {
				other.ptr_ = nullptr;
			}
			shared_ptr_holder& operator=(shared_ptr_holder&& other)noexcept {
				impl_ = std::move(other.impl_);
				ptr_ = std::move(other.ptr_);
				other.ptr_ = nullptr;
				return *this;
			}
			template<typename T>
			shared_ptr_holder(T t, value_tag) :impl_(std::make_shared<const holder_impl<T>>(std::move(t))), ptr_(get_ptr_impl()) {}
			shared_ptr_holder(const value_holder& v) :impl_(v.clone_ptr()), ptr_(get_ptr_impl()) {}
			auto clone_ptr()const { return impl_ ? impl_->clone() : nullptr; }
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
		std::conditional_t<
		std::conjunction_v<detail::is_const_signature<Signatures>...>,
		detail::shared_ptr_holder, detail::value_holder >,
		std::make_index_sequence<sizeof...(Signatures)>, Signatures...>;

} // namespace polymorphic
