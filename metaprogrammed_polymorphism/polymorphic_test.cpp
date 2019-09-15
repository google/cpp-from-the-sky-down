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

#include <gmock/gmock.h>
#include <string>
#include "polymorphic.hpp"

struct x2 {};

void poly_extend(x2, int& i) { i *= 2; }
void poly_extend(x2, std::string& s) { s = s + s; }

TEST(Polymorphic, MutableRef) {
	std::string s("hello");
	int i = 5;

	polymorphic::ref<void(x2)> r{ s };
	r.call<x2>();
	r = i;
	r.call<x2>();

	EXPECT_THAT(i, 10);
	EXPECT_THAT(s, "hellohello");

}

TEST(Polymorphic, CopyMutableRef) {
	std::string s("hello");
	int i = 5;

	polymorphic::ref<void(x2)> r{ s };
	r.call<x2>();
	r = i;
	r.call<x2>();
	auto r2 = r;
	r2.call<x2>();

	EXPECT_THAT(i, 20);
	EXPECT_THAT(s, "hellohello");

}



int main(int argc, char **argv) {
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
