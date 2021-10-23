#pragma once
#include <nlohmann/json.hpp>

#include "tagged_tuple.h"

namespace nlohmann {
template <typename... Members>
struct adl_serializer<ftsd::tagged_tuple<Members...>> {
  using TTuple = ftsd::tagged_tuple<Members...>;

  template <typename Member>
  static auto get_from_json(const json& j) {
    using ftsd::tag;
    if constexpr (!Member::has_default_init()) {
      return tag<Member::fixed_key()> =
                 j.at(Member::key().data()).get<typename Member::value_type>();
    } else {
      if (j.contains(Member::key().data())) {
        return tag<Member::fixed_key()> =
                   std::optional<typename Member::value_type>(
                       j.at(Member::key().data()).get<typename Member::value_type>());
      } else {
        return tag<Member::tag_type::value> = std::optional<typename Member::value_type>();
      }
    }
  }
  static TTuple from_json(const json& j) {
    return TTuple::apply_static([&]<typename... M>(M * ...) {
      return TTuple(get_from_json<M>(j)...);
    });
  }

  static void to_json(json& j, const TTuple& t) {
    t.for_each([&](auto& member) { j[member.key().data()] = member.value(); });
  }
};
}  // namespace nlohmann
