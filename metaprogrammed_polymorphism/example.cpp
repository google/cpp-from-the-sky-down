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

#include "polymorphic.hpp"

#include <iostream>
#include <string>
#include <vector>
#include <memory>

class draw {};
class x2 {};

// Use types instead of names
// void draw(std::ostream&) -> void(draw, std::ostream&)
void call_draw(polymorphic::ref<void(draw, std::ostream&) const> d) {
	std::cout << "in call_draw\n";
	d.call<draw>(std::cout);
}


template <typename T>
void poly_extend(draw, const T& t, std::ostream& os) {
	os << t << "\n";
}

class x2;
template <typename T>
void poly_extend(x2, T& t, std::unique_ptr<int>) {
	t = t + t;
}

int main() {
	std::vector<polymorphic::object<
		void(x2,std::unique_ptr<int>),
		void(draw, std::ostream & os) const
		>> objects;
	for (int i = 0; i < 30; ++i) {
		switch (i % 3) {
		case 0:
			objects.emplace_back(i);
			break;
		case 1:
			objects.emplace_back(double(i) + double(i) / 10.0);
			break;
		case 2:
			objects.emplace_back(std::to_string(i) + " string");
			break;
		}
	}
	auto o = objects.front();
	polymorphic::object<void(draw, std::ostream&)const> co(10);
	auto co1 = co;
	for (const auto& o : objects) call_draw(o);
	for (auto& o : objects) o.call<x2>(nullptr);
	for (auto& o : objects) call_draw(o);
}
