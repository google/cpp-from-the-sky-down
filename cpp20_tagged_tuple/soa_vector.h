#pragma once
#include <boost/stl_interfaces/iterator_interface.hpp>
#include <span>
#include <vector>

#include "tagged_tuple.h"

namespace ftsd {

template <typename TaggedTuple>
class soa_vector;

template <auto... Tags, typename... Ts, auto... Inits>
class soa_vector<tagged_tuple<member<Tags, Ts, Inits>...>> {
  using TaggedTuple = tagged_tuple<member<Tags, Ts, Inits>...>;
  tagged_tuple<member<
      Tags, std::vector<tagged_tuple_value_type_t<Tags, TaggedTuple>>>...>
      vectors_;

  template <auto Tag, auto...>
  decltype(auto) first_helper() {
    return get<Tag>(vectors_);
  }

  template <auto Tag, auto...>
  decltype(auto) first_helper() const {
    return get<Tag>(vectors_);
  }

  decltype(auto) first() { return first_helper<Tags...>(); }

  decltype(auto) first() const { return first_helper<Tags...>(); }

 public:
  soa_vector() = default;
  decltype(auto) vectors() { return (vectors_); }
  decltype(auto) vectors() const { return (vectors_); }

  void push_back(TaggedTuple t) {
    (ftsd::get<Tags>(vectors_).push_back(get<Tags>(t)), ...);
  }

  void pop_back() { (get<Tags>(vectors_).pop_back(), ...); }

  void clear() { (get<Tags>(vectors_).clear(), ...); }

  std::size_t size() const { return first().size(); }

  bool empty() const { return first().empty(); }

  auto operator[](std::size_t i) {
    return tagged_tuple_ref_t<TaggedTuple>(
        (ftsd::tag<Tags> = std::ref(ftsd::get<Tags>(vectors_)[i]))...);
  }

  auto operator[](std::size_t i) const {
    return tagged_tuple_ref_t<TaggedTuple>(
        (ftsd::tag<Tags> = std::ref(ftsd::get<Tags>(vectors_)[i]))...);
  }

  auto front() { return (*this)[0]; }
  auto back() { return (*this)[size() - 1]; }
};

template <typename Tag, typename TaggedTuple>
decltype(auto) get_impl(soa_vector<TaggedTuple>& s) {
  return std::span{ftsd::get<Tag::value>(s.vectors())};
}

template <typename Tag, typename TaggedTuple>
auto get_impl(const soa_vector<TaggedTuple>& s) {
  return std::span{ftsd::get<Tag::value>(s.vectors())};
}

template <typename Tag, typename TaggedTuple>
auto get_impl(soa_vector<TaggedTuple>&& s) {
  return std::span{ftsd::get<Tag::value>(std::move(s.vectors()))};
}

}  // namespace ftsd
