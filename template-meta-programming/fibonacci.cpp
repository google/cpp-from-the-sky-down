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
// See the License for the specific language governing permissions and
// limitations under the License.


#include <iostream>
#include <type_traits>	


namespace specialization {
	template <int I>
	struct fib {
		enum { value = fib<I - 1>::value + fib<I - 2>::value };
	};

	template<>
	struct fib<0> {
		enum { value = 1 };
	};

	template<>
	struct fib<1> {
		enum { value = 1 };
	};

}

namespace constexpr_function {
	constexpr int fibonacci(int i) {
		return (i < 2) ? 1 :
			fibonacci(i - 1) + fibonacci(i - 2);
	}
}

namespace constexpr_function_iterative {
	constexpr int fibonacci(int i) {
		if (i < 2) return 1;
		auto fib_prev = 1;
		auto fib_cur = 1;
		for (; i > 1; --i) {
			auto fib_next = fib_prev + fib_cur;
			fib_prev = fib_cur;
			fib_cur = fib_next;
		}
		return fib_cur;
	}
}


namespace templated {
	template<typename T>
	struct make_ptr {
		using type = T * ;
	};

	template<typename T>
	struct make_ptr<T*> {
		using type = T * ;
	};
}

template<typename T>
struct t2t {
	using type = T;
};
namespace function {
	template<typename T>
	auto make_ptr(t2t<T>) {
		if constexpr (std::is_pointer_v<T>) {
			return t2t<T>{};
		}
		else {
			return t2t<T*>{};
		}
	}
}






int main() {

	static_assert(specialization::fib<4>::value == 5);
	static_assert(constexpr_function::fibonacci(4) == 5);
	static_assert(constexpr_function_iterative::fibonacci(4) == 5);
	static_assert(std::is_same_v<templated::make_ptr<int>::type, int*>);
	static_assert(std::is_same_v<templated::make_ptr<int*>::type, int*>);
	static_assert(std::is_same_v < decltype(function::make_ptr(t2t<int>{}))::type, int* > );
	static_assert(std::is_same_v < decltype(function::make_ptr(t2t<int*>{}))::type, int* > );



}