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

#include <vector>
#include <algorithm>
#include <iostream>
#include <iterator>


template<typename V, typename T>
void insert_sorted(V& v, T&& t) {
	v.push_back(std::forward<T>(t));
	auto last = std::prev(v.end());
	auto pos = std::lower_bound(v.begin(), last, *last);
	std::rotate(pos, last, v.end());
}


template<typename V, typename T>
void nearest_k(V& v, const T& t, int k) {
	if (v.size() <= k) return;
	auto comp = [&t](auto& a, auto& b) {
		return std::abs(a - t) < std::abs(b - t);
	};

	auto iter = v.begin() + k;
	std::nth_element(v.begin(),iter , v.end(),comp);
	if (iter != v.end()) {
		v.erase(iter, v.end());
	}
}


int main() {
	std::vector<int> v;

	insert_sorted(v, 5);
	insert_sorted(v, 1);
	insert_sorted(v, 2);
	insert_sorted(v, 4);

	insert_sorted(v, 3);

	std::copy(v.begin(), v.end(), std::ostream_iterator<int>(std::cout, "\n"));
	std::cout << "\n\n";

	nearest_k(v, 5, 2);
	std::copy(v.begin(), v.end(), std::ostream_iterator<int>(std::cout, "\n"));



}