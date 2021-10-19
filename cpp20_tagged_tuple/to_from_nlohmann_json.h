#pragma once
#include "tagged_tuple.h"

#include <nlohmann/json.hpp>



namespace nlohmann {
	template<typename... Members>
	struct adl_serializer<ftsd::tagged_tuple<Members...>> {
		using TTuple = ftsd::tagged_tuple<Members...>;
		// note: the return type is no longer 'void', and the method only takes
		// one argument

		template<typename Member>
			requires requires{ { Member::init()}; }
		static auto get_from_json(const json& j) {
			using ftsd::tag;
			auto key = std::string(Member::key());
			if constexpr (std::is_same_v<decltype(Member::init()), void>) {
				return tag<Member::tag_type::value> = j.at(key).get<typename Member::value_type>();
			}
			else {
				if (j.contains(key)) {
					return tag<Member::tag_type::value> = j.at(key).get<typename Member::value_type>();
				}
				else {
					return tag<Member::tag_type::value> = Member::init();

				}
			}
		}
		static TTuple from_json(const json& j) {
			using ftsd::tag;
			using ftsd::member_to_impl_t;

			return TTuple(get_from_json<ftsd::member_to_impl_t<TTuple, Members>>(j) ...);
		}

		// Here's the catch! You must provide a to_json method! Otherwise you
		// will not be able to convert move_only_type to json, since you fully
		// specialized adl_serializer on that type
		static void to_json(json& j, const TTuple& t) {
			t.for_each([&](auto& member) {
				j[std::string(member.key())] = member.value();
				});
		}
	};
}
