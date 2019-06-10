// Copyright 2019 Google LLC
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
#include "tagged_tuple.h"

int main() {
	using namespace tagged_tuple;
  auto t =
      make_tagged_tuple(member<class A, int>{1}, member<class B, double>{2});


  for_each(t, [](auto& t) { std::cout << t.value; });

  auto nt = append(t, member<class C, int>{3});
  for_each(nt, [](auto& t) { std::cout << t.value; });

  static_assert(tuple_size(nt) == 3);

  static_assert(has_tag<C, decltype(nt)>);



}




