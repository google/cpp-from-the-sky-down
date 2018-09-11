#pragma once
#include <memory>
#include <iostream>
namespace my_shapes {

	struct empty_shape {};

	template <typename T>
	void draw_implementation(T& t) {
		t.draw();
	}

	inline void draw_implementation(empty_shape) { std::cout << "empty\n"; }

	class shape {
	public:
		shape() = default;
		template <typename T>
		explicit shape(T&& t)
			: ptr_(std::make_unique<shape_implementation<T>>(std::forward<T>(t))) {}
		shape(shape&&) = default;
		shape(const shape& other) : ptr_(other.ptr_->clone()) {}
		shape& operator=(shape&&) = default;
		shape& operator=(const shape& other) {
			auto new_shape = other;
			(*this) = std::move(new_shape);
			return *this;
		}

		void draw()const {
			if (ptr_)
				ptr_->draw_();
			else
				draw_implementation(empty_shape{});
		}

	private:
		struct shape_interface {
			virtual void draw_() const = 0;
			virtual std::unique_ptr<shape_interface> clone() const = 0;
			~shape_interface() = default;
		};

		template <typename T>
		struct shape_implementation : shape_interface {
			void draw_() const override {
				draw_implementation(t_);
			}
			std::unique_ptr<shape_interface> clone() const override {
				return std::make_unique<shape_implementation>(t_);
			}
			shape_implementation() = default;
			template <typename U>
			explicit shape_implementation(U&& u) : t_(std::forward<U>(u)) {}

			T t_;
		};

		std::unique_ptr<shape_interface> ptr_;
	};

}  // namespace shapes

