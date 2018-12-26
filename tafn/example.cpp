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

#include <iostream>
#include "tafn.hpp"
#include <vector>
#include <algorithm>



struct multiply {};
struct add {};


void customization_point(multiply, tafn::type<int>, int& i, int value) {
	i *= value;
}
int customization_point(add, tafn::type<int>, const int& i, int value) {
	return i + value;
}


namespace algs {
	struct sort {};
	struct unique {};
	struct copy {};

	template<typename T>
	decltype(auto) customization_point(sort, tafn::all_types, T&& t) {
		std::sort(t.begin(), t.end());
		return t;
	}
	template<typename T>
	decltype(auto) customization_point(unique, tafn::all_types, T&& t) {
		auto iter = std::unique(t.begin(), t.end());
		t.erase(iter, t.end());
		return t;
	}
	template<typename T, typename I>
	decltype(auto) customization_point(copy, tafn::all_types, T&& t, I iter) {
		std::copy(t.begin(), t.end(), iter);
		return t;
	}



}


#include <iterator>
int main() {


	int i{ 5 };

	tafn::wrap(i)._<multiply>(10);

	std::cout << i;

	int j = 9;
	auto k = tafn::wrap(i)._<add>(2)._<add>(3).unwrapped;
	std::cout << k << " " << tafn::wrap(j)._<add>(4)._<add>(5).unwrapped << " ";

	std::vector<int> v{ 4, 4, 1, 2, 2, 9, 9, 9, 7, 6, 6, };
	for (auto& i : tafn::wrap(v)._<algs::sort>()._<algs::unique>().unwrapped) {
		std::cout << i << " ";
	}





}