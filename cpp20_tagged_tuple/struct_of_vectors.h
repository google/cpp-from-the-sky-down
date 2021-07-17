#pragma once
#include <boost/stl_interfaces/iterator_interface.hpp>
#include <vector>
#include <span>

#include "tagged_tuple.h"

namespace ftsd {

template <typename TaggedTuple>
class struct_of_vectors;

template <auto... Tags, typename... Ts, auto... Inits>
class struct_of_vectors<tagged_tuple<member<Tags, Ts, Inits>...>> {
  using TaggedTuple = tagged_tuple<member<Tags, Ts, Inits>...>;
  tagged_tuple<member<
      Tags, std::vector<internal_tagged_tuple::t_or_auto_t<TaggedTuple, Ts, Inits>>>...>
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
  struct_of_vectors() = default;
  decltype(auto) vectors(){return (vectors_);}
  decltype(auto) vectors()const{return (vectors_);}

  void push_back(TaggedTuple t) {
    (ftsd::get<Tags>(vectors_).push_back(get<Tags>(t)), ...);
  }

  void pop_back() { (get<Tags>(vectors_).pop_back(), ...); }

  void clear() { (get<Tags>(vectors_).clear(), ...); }

  std::size_t size() const { return first().size(); }

  bool empty() const { return first().empty(); }

  auto operator[](std::size_t i) {
    return tagged_tuple_ref_t<TaggedTuple>(
        internal_tagged_tuple::member_impl<
           internal_tagged_tuple::tuple_tag<Tags>,
            std::add_lvalue_reference_t<internal_tagged_tuple::t_or_auto_t<TaggedTuple,Ts,Inits>>,
            []{}>(ftsd::get<Tags>(vectors_)[i])...);
  }

  auto operator[](std::size_t i) const {
    return tagged_tuple_ref_t<TaggedTuple>(
        internal_tagged_tuple::member_impl<
            internal_tagged_tuple::tuple_tag<Tags>,
            std::add_lvalue_reference_t<
                const internal_tagged_tuple::t_or_auto_t<TaggedTuple,Ts,Inits>>,
            []{}>(ftsd::get<Tags>(vectors_)[i])...);
  }

  auto front() { return (*this)[0]; }
  auto back() { return (*this)[size() - 1]; }
};

  template <typename Tag, typename TaggedTuple>
  decltype(auto) get_impl(struct_of_vectors<TaggedTuple>& s) {
    return std::span{ftsd::get<Tag::value>(s.vectors())};
  }


  template <typename Tag, typename TaggedTuple>
  auto get_impl(const struct_of_vectors<TaggedTuple>& s) {
    return std::span{ftsd::get<Tag::value>(s.vectors())};
  }

  template <typename Tag, typename TaggedTuple>
  auto get_impl(struct_of_vectors<TaggedTuple>&& s) {
    return std::span{ftsd::get<Tag::value>(std::move(s.vectors()))};
  }



}  // namespace ftsd
