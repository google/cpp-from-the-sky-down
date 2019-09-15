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
struct stupid_hash {};

void poly_extend(x2, int& i) { i *= 2; }
void poly_extend(x2, std::string& s) { s = s + s; }

int poly_extend(stupid_hash, const int& i) { return i; }
int poly_extend(stupid_hash, const std::string& s) { return static_cast<int>(s.size()); }



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

TEST(Polymorphic, MutableObject) {
	std::string s("hello");
	int i = 5;

	polymorphic::object<void(x2)> o{ s };
	o.call<x2>();
	const auto& s_ref = *static_cast<std::string*>(o.get_ptr());
	EXPECT_THAT(s_ref, "hellohello");
	o = i;
	o.call<x2>();

	const auto& i_ref = *static_cast<int*>(o.get_ptr());
	EXPECT_THAT(i_ref, 10);

}

TEST(Polymorphic, CopyMutableObject) {
	std::string s("hello");
	int i = 5;

	polymorphic::object<void(x2)> o{ s };
	auto o2 = o;

	o.call<x2>();
	const auto& s_ref = *static_cast<std::string*>(o.get_ptr());
	const auto& s_ref2 = *static_cast<std::string*>(o2.get_ptr());
	EXPECT_THAT(s_ref, "hellohello");
	EXPECT_THAT(s_ref2, "hello");
	o = i;
	o2 = o;
	o2.call<x2>();

	const auto& i_ref = *static_cast<int*>(o.get_ptr());
	const auto& i_ref2 = *static_cast<int*>(o2.get_ptr());
	EXPECT_THAT(i_ref, 5);
	EXPECT_THAT(i_ref2, 10);
}

TEST(Polymorphic, ConstRef) {
	const std::string s("hello");
	const int i = 5;

	polymorphic::ref<int(stupid_hash)const> r{ s };
	EXPECT_THAT(r.call<stupid_hash>(),static_cast<int>(s.size()));

	r = i;
	EXPECT_THAT(r.call<stupid_hash>(),i);
}

TEST(Polymorphic, CopyConstRef) {
	std::string s("hello");
	const int i = 5;

	polymorphic::ref<int(stupid_hash)const> r{ std::as_const(s) };
	EXPECT_THAT(r.call<stupid_hash>(),static_cast<int>(s.size()));
	auto r2 = r;

	s = "jrb";
	EXPECT_THAT(r.call<stupid_hash>(),static_cast<int>(s.size()));
	EXPECT_THAT(r2.call<stupid_hash>(),static_cast<int>(s.size()));

	r = i;
	EXPECT_THAT(r.call<stupid_hash>(),i);
	EXPECT_THAT(r2.call<stupid_hash>(),static_cast<int>(s.size()));
}

TEST(Polymorphic, CopyConstRefFromMutable) {
	std::string s("hello world");
	const int i = 5;

	polymorphic::ref<void(x2),int(stupid_hash)const> r{ s };
	EXPECT_THAT(r.call<stupid_hash>(),static_cast<int>(s.size()));

	polymorphic::ref<int(stupid_hash)const> r2{ i };

	EXPECT_THAT(r2.call<stupid_hash>(),i);
	r2 = r;
	EXPECT_THAT(r2.call<stupid_hash>(),static_cast<int>(s.size()));
	r.call<x2>();
	EXPECT_THAT(r2.call<stupid_hash>(),static_cast<int>(s.size()));
	EXPECT_THAT(s, "hello worldhello world");
}

TEST(Polymorphic, CopyMutableRefFromObject) {
	std::string s("hello");
	int i = 5;

	polymorphic::object<void(x2)> o{ s };
	o.call<x2>();
	o = i;
	o.call<x2>();
	auto r2 = o;
	r2.call<x2>();

	EXPECT_THAT(i, 20);
	EXPECT_THAT(s, "hellohello");

}




int main(int argc, char **argv) {
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
