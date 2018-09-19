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

#include "units.hpp"
#include <iostream>

int main() {
  // A 3.4m high block is stacked on a 7.1m bock. Drop a 5.5kg weight is droped
  // from the top of the stack. How long will it take before it hits the ground,
  // and what will be force it exerts on the ground, and how much kinetic energy
  // will it have.

  auto mass = 5.5_kg;
  auto height = 3.4_m + 7.1_m;
  auto g = 9.8_m / pow<2>(1.0_s);

  // height = 1/2 a * t^2
  // 2*height/a = t^2

  auto t = sqrt(2 * height / g);

  auto velocity = t * g;

  // Does not compile
  //  auto v = mass + g;

  std::cout << "Time to fall: " << t << "\n";
  std::cout << "Velocity prior to impact: " << velocity << "\n";
  std::cout << "Force of impact: " << mass * g << "\n";
  std::cout << "Kinetic energy at impact: " << 0.5 * mass * pow<2>(velocity)
            << "\n";
}
