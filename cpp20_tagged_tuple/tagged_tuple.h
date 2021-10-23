#pragma once
#include <algorithm>
#include <array>
#include <cstdint>
#include <optional>
#include <string_view>
#include <tuple>
#include <utility>

namespace ftsd {

namespace internal_tagged_tuple {

template <std::size_t N>
struct fixed_string {
  constexpr fixed_string(const char (&foo)[N + 1]) {
    std::copy_n(foo, N + 1, data);
  }
  constexpr fixed_string(std::string_view s) {
    std::copy_n(s.data(), N, data);
  };
  auto operator<=>(const fixed_string&) const = default;
  char data[N + 1] = {};
  constexpr std::string_view sv() const {
    return std::string_view(&data[0], N);
  }
  constexpr std::size_t size() const { return N; }
  constexpr auto operator[](std::size_t i) const { return data[i]; }
};

template <std::size_t N>
fixed_string(const char (&str)[N]) -> fixed_string<N - 1>;

template <std::size_t N>
fixed_string(fixed_string<N>) -> fixed_string<N>;

template <typename Member>
constexpr auto member_tag() {
  return Member::fs.sv();
}

template <typename T>
struct type_to_type {
  using type = T;
};

template <typename... Members>
struct tagged_tuple;

template <typename S, typename M>
struct chop_to_helper;

template <typename... Members, typename M>
struct chop_to_helper<tagged_tuple<Members...>, M> {
  template <fixed_string fs>
  static constexpr std::size_t find_index() {
    std::array<std::string_view, sizeof...(Members)> ar{
        member_tag<Members>()...};
    for (std::size_t i = 0; i < ar.size(); ++i) {
      if (ar[i] == fs.sv()) return i;
    }
  }

  static constexpr std::size_t index = find_index<M::fs>();
  decltype(std::make_index_sequence<index>()) sequence;
  using member_tuple = std::tuple<Members...>;
  template <std::size_t... I>
  static constexpr auto chop(std::index_sequence<I...>) {
    return type_to_type<
        tagged_tuple<std::tuple_element_t<I, member_tuple>...>>{};
  }

  using type = typename decltype(chop(sequence))::type;
};

template <typename S, typename M>
using chopped = typename chop_to_helper<S, M>::type;

template <typename T>
struct default_init_impl {
  static constexpr auto init() { if constexpr (std::is_default_constructible_v<T>) return T{}; }
};

template <typename T>
struct default_init_impl<T&> {
  static constexpr void init() {}
};

template <typename T>
constexpr auto default_init = []() { return default_init_impl<T>::init(); };

struct dummy_conversion {};

struct no_conversion {
  no_conversion() = delete;
  no_conversion(const no_conversion&) = delete;
  no_conversion& operator=(const no_conversion&) = delete;
};

template <typename T>
struct optional_no_ref {
  using type = std::optional<T>;
};
template <typename T>
struct optional_no_ref<T&> {
  using type = no_conversion;
};

template <typename T>
using optional_no_ref_t = typename optional_no_ref<T>::type;

template <typename Tag, typename T, auto Init = default_init<T>>
struct member_impl {
  static constexpr decltype(Init) init = Init;
  T value_;
  template <typename Self>
  constexpr member_impl(Self& self, dummy_conversion) requires requires {
    { Init() } -> std::convertible_to<T>;
  } : value_(Init()) {
  }

  static constexpr bool has_default_init() requires requires {
    { Init() } -> std::same_as<void>;
  }
  { return false; }

  static constexpr bool has_default_init() { return true; }

  using optional_type = optional_no_ref_t<T>;
  template <typename Self, typename U>
  constexpr member_impl(Self& self, U value_or) requires
      std::same_as<U, optional_type> && requires {
    { Init() } -> std::convertible_to<T>;
  } : value_(value_or.has_value() ? std::move(*value_or) : Init()) {
  }

  template <typename Self>
  constexpr member_impl(Self& self, dummy_conversion) requires requires {
    { Init() } -> std::same_as<void>;
  }
  {
    static_assert(!std::is_same_v<decltype(Init()), void>,
                  "Missing required argument.");
  }

  template <typename Self>
  constexpr member_impl(Self& self, dummy_conversion) requires requires {
    { Init(self) } -> std::convertible_to<T>;
  } : value_(Init(self)) {
  }
  template <typename Self, typename U>
  constexpr member_impl(Self& self, U value_or) requires
      std::same_as<U, optional_type> && requires {
    { Init(self) } -> std::convertible_to<T>;
  } : value_(value_or.has_value() ? std::move(*value_or) : Init(self)) {
  }
  constexpr member_impl(T value) requires(!std::is_reference_v<T>)
      : value_(std::move(value)) {}
  constexpr member_impl(T value) requires(std::is_reference_v<T>)
      : value_(value) {}
  template <typename Self>
  constexpr member_impl(Self&, T value) requires(!std::is_reference_v<T>)
      : value_(std::move(value)) {}
  template <typename Self>
  constexpr member_impl(Self&, T value) requires(std::is_reference_v<T>)
      : value_(value) {}

  constexpr member_impl() requires requires {
    { Init() } -> std::convertible_to<T>;
  } : value_(Init()) {
  }
  template <typename Self>
  constexpr member_impl(Self& self) requires requires {
    { Init(self) } -> std::convertible_to<T>;
  } : value_(Init(self)) {
  }
  constexpr member_impl(const member_impl&) = default;
  constexpr member_impl& operator=(const member_impl&) = default;
  constexpr member_impl(member_impl&&) = default;
  constexpr member_impl& operator=(member_impl&&) = default;
  template <typename Self, typename OtherT, auto OtherInit>
  constexpr member_impl(Self& self,
                        const member_impl<Tag, OtherT, OtherInit>& other)
      : member_impl(self, other.value_) {}
  template <typename Self, typename OtherT, auto OtherInit>
  constexpr member_impl(Self& self, member_impl<Tag, OtherT, OtherInit>& other)
      : member_impl(self, other.value_) {}

  template <typename Self, typename OtherT, auto OtherInit>
  constexpr member_impl(Self& self, member_impl<Tag, OtherT, OtherInit>&& other)
      : member_impl(self, std::move(other.value_)){};
  template <typename OtherT, auto OtherInit>
  constexpr member_impl& operator=(
      const member_impl<Tag, OtherT, OtherInit>& other) {
    value_ = other.value_;
    return *this;
  }
  template <typename OtherT, auto OtherInit>
  constexpr member_impl& operator=(
      member_impl<Tag, OtherT, OtherInit>&& other) {
    value_ = std::move(other.value_);
    return *this;
  };

  constexpr auto operator<=>(const member_impl&) const = default;

  using tag_type = Tag;
  using value_type = T;

  static constexpr std::string_view key() { return Tag::value.sv(); }
  static constexpr auto fixed_key() { return Tag::value; }

  constexpr value_type& value() & { return value_; }
  constexpr value_type&& value() && { return std::move(value_); }
  constexpr const value_type& value() const& { return value_; }
  constexpr const value_type&& value() const&& { return std::move(value_); }
};
template <fixed_string fs>
struct tuple_tag {
  static constexpr decltype(fs) value = fs;
  static constexpr bool ftsd_is_tuple_tag_type = true;
  template <typename T>
  constexpr auto operator=(T t) const {
    return member_impl<tuple_tag<fixed_string<fs.size()>(fs)>, T>{std::move(t)};
  }
  template <typename T>
  constexpr decltype(auto) operator()(T&& t) const {
    return get<fs>(std::forward<T>(t));
  }
};

template <typename T>
struct is_tuple_tag : std::false_type {};

template <typename T>
requires requires { {T::ftsd_is_tuple_tag_type}; }
struct is_tuple_tag<T> : std::true_type {};

template <typename T>
constexpr bool is_tuple_tag_v = is_tuple_tag<T>::value;

struct auto_;

template <typename Self, typename T, auto Init>
struct t_or_auto {
  using type = T;
};

template <typename Self, auto Init>
requires requires { {Init()}; }
struct t_or_auto<Self, auto_, Init> {
  using type = decltype(Init());
};

template <typename Self, typename T, auto Init>
requires requires {
  { Init(std::declval<Self&>()) } -> std::convertible_to<T>;
}
struct t_or_auto<Self, T, Init> {
  using type = decltype(Init(std::declval<Self&>()));
};

template <typename Self, auto Init>
requires requires { {Init(std::declval<Self&>())}; }
struct t_or_auto<Self, auto_, Init> {
  using type = decltype(Init(std::declval<Self&>()));
};

template <typename Self, typename T, auto Init>
using t_or_auto_t = typename t_or_auto<Self, T, Init>::type;

template <fixed_string Fs, typename T, auto Init = default_init<T>>
struct member {
  using type = T;
  using fs_type = decltype(Fs);
  static constexpr fs_type fs = Fs;
  static constexpr decltype(Init) init = Init;
};

template <typename Self, typename Member>
struct member_to_impl {
  using type =
      member_impl<tuple_tag<fixed_string<Member::fs.size()>(Member::fs)>,
                  typename t_or_auto<chopped<Self, Member>,
                                     typename Member::type, Member::init>::type,
                  Member::init>;
};

template <typename Self, typename Member>
using member_to_impl_t = typename member_to_impl<Self, Member>::type;

template <typename Tag, typename T>
constexpr auto make_member_impl(T t) {
  return member_impl<Tag, T>{std::move(t)};
}

template <typename... Members>
struct parameters : Members... {
  constexpr operator dummy_conversion() { return {}; }
};

template <typename... Members>
parameters(Members&&...) -> parameters<std::decay_t<Members>...>;

template <typename Self, typename... Members>
struct tagged_tuple_base : member_to_impl_t<Self, Members>... {
  template <typename... Args>
  constexpr tagged_tuple_base(Self& self, parameters<Args...> p)
      : member_to_impl_t<Self, Members>{self, p}... {}

  template <typename OtherSelf, typename... OtherMembers>
  constexpr tagged_tuple_base(
      tagged_tuple_base<OtherSelf, OtherMembers...>& other)

      : member_to_impl_t<Self, Members>{
            other, static_cast<member_to_impl_t<OtherSelf, OtherMembers>&>(
                       other)}... {}

  template <typename OtherSelf, typename... OtherMembers>
  constexpr tagged_tuple_base(
      const tagged_tuple_base<OtherSelf, OtherMembers...>& other)

      : member_to_impl_t<Self, Members>{
            other,
            static_cast<const member_to_impl_t<OtherSelf, OtherMembers>&>(
                other)}... {}

  constexpr tagged_tuple_base(const tagged_tuple_base&) = default;
  constexpr tagged_tuple_base& operator=(const tagged_tuple_base&) = default;

  constexpr tagged_tuple_base(tagged_tuple_base&&) = default;
  constexpr tagged_tuple_base& operator=(tagged_tuple_base&&) = default;

  constexpr auto operator<=>(const tagged_tuple_base&) const = default;

  template <typename F>
  constexpr static auto apply_static(F&& f) {
    return f(static_cast<member_to_impl_t<Self, Members>*>(nullptr)...);
  }

  template <typename F>
  constexpr auto apply(F&& f) & {
    return f(static_cast<member_to_impl_t<Self, Members>&>(*this)...);
  }
  template <typename F>
  constexpr auto apply(F&& f) const& {
    return f(static_cast<const member_to_impl_t<Self, Members>&>(*this)...);
  }

  template <typename F>
  constexpr auto apply(F&& f) && {
    return f(static_cast<member_to_impl_t<Self, Members>&&>(*this)...);
  }

  template <typename F>
  constexpr void for_each(F&& f) & {
    auto functor = [&](auto&&... a) mutable {
      (f(std::forward<decltype(a)>(a)), ...);
    };
    apply(functor);
  }

  template <typename F>
  constexpr void for_each(F&& f) const& {
    auto functor = [&](auto&&... a) mutable {
      (f(std::forward<decltype(a)>(a)), ...);
    };
    apply(functor);
  }

  template <typename F>
  constexpr void for_each(F&& f) && {
    auto functor = [&f](auto&&... a) mutable {
      (f(std::forward<decltype(a)>(a)), ...);
    };
    apply(functor);
  }

  static constexpr auto size() { return sizeof...(Members); }
};

template <typename Tag, typename T, auto Init>
constexpr decltype(auto) get_impl(member_impl<Tag, T, Init>& m) {
  return (m.value());
}

template <typename Tag, typename T, auto Init>
constexpr decltype(auto) get_impl(const member_impl<Tag, T, Init>& m) {
  return (m.value());
}

template <typename Tag, typename T, auto Init>
constexpr decltype(auto) get_impl(member_impl<Tag, T, Init>&& m) {
  return std::move(m.value());
}

template <typename Tag, typename T, auto Init>
constexpr decltype(auto) get_impl(const member_impl<Tag, T, Init>&& m) {
  return std::move(m.value());
}

template <fixed_string fs, typename S>
constexpr decltype(auto) get(S&& s) {
  return get_impl<tuple_tag<fixed_string<fs.size()>(fs)>>(std::forward<S>(s));
}

template <typename... Members>
struct tagged_tuple : tagged_tuple_base<tagged_tuple<Members...>, Members...> {
  using super = tagged_tuple_base<tagged_tuple, Members...>;

  template <typename... Tag, typename... T, auto... Init>
  constexpr tagged_tuple(member_impl<Tag, T, Init>... args)
      : super(*this, parameters{std::move(args)...}) {}

  constexpr tagged_tuple() : super(*this, parameters{}) {}
  template <typename... OtherMembers>
  constexpr tagged_tuple(tagged_tuple<OtherMembers...>& other) : super(other) {}
  template <typename... OtherMembers>
  constexpr tagged_tuple(const tagged_tuple<OtherMembers...>& other)
      : super(other) {}
  constexpr tagged_tuple(const tagged_tuple& other) = default;
  constexpr tagged_tuple& operator=(const tagged_tuple&) = default;
  constexpr tagged_tuple(tagged_tuple&&) = default;
  constexpr tagged_tuple& operator=(tagged_tuple&& other) = default;

  template <typename Tag>
  constexpr auto& operator[](Tag) {
    return get<Tag::value>(*this);
  }

  template <typename Tag>
  constexpr auto& operator[](Tag) const {
    return get<Tag::value>(*this);
  }
};

template <typename Tag, typename T, auto Init>
T tagged_tuple_value_type_impl(member_impl<Tag, T, Init>&);

// A lambda passed to a template must be stateless and default constructible
// anyway.
template <typename Tag, typename T, auto Init>
decltype(Init) tagged_tuple_init_impl(member_impl<Tag, T, Init>&);

template <fixed_string fs, typename Tuple>
using tagged_tuple_value_type_t =
    decltype(tagged_tuple_value_type_impl<tuple_tag<fs>> (std::declval<Tuple&>()));

template <fixed_string fs, typename Tuple>
inline constexpr auto tagged_tuple_init_v =
    decltype(tagged_tuple_init_impl<tuple_tag<fs>>(std::declval<Tuple&>())){};


template <typename Member>
struct member_impl_to_member {
  using tag_t = typename Member::tag_type;
  static constexpr decltype(Member::init) init = Member::init;
  static constexpr fixed_string<tag_t::value.size()> fs{tag_t::value.sv()};
  using type = member<fs, typename Member::value_type, init>;
};

template <typename T>
using member_impl_to_member_t = typename member_impl_to_member<T>::type;

template <typename... Tag, typename... T, auto... Init>
tagged_tuple(member_impl<Tag, T, Init>...)
    -> tagged_tuple<member_impl_to_member_t<member_impl<Tag, T, Init>>...>;

template <typename TaggedTuple>
struct tagged_tuple_ref;

template <auto... Tags, typename... T, auto... Init>
struct tagged_tuple_ref<tagged_tuple<member<Tags, T, Init>...>> {
  using Self = tagged_tuple<member<Tags, T, Init>...>;
  using type = tagged_tuple<member<
      Tags,
      std::add_lvalue_reference_t<typename t_or_auto<Self, T, Init>::type>,
      Init>...>;
};

template <auto... Tags, typename... T, auto... Init>
struct tagged_tuple_ref<const tagged_tuple<member<Tags, T, Init>...>> {
  using Self = tagged_tuple<member<Tags, T, Init>...>;
  using type =
      tagged_tuple<member<Tags,
                          std::add_lvalue_reference_t<std::add_const_t<
                              typename t_or_auto<Self, T, Init>::type>>,
                          Init>...>;
};

template <typename TaggedTuple>
using tagged_tuple_ref_t = typename tagged_tuple_ref<TaggedTuple>::type;

template <fixed_string fs>
inline constexpr auto tag = tuple_tag<fixed_string<fs.size()>(fs)>{};

enum class tag_comparison { eq, ne, lt, gt, le, ge };

template <typename TagOrValue1, typename TagOrValue2, tag_comparison comparison>
struct tag_comparator_predicate {
  template <typename Value, typename TS>
  constexpr static const auto& get_value_for_comparison(const Value& value,
                                                        const TS&) {
    return value;
  }

  template <typename Tag, typename TS>
  requires is_tuple_tag_v<Tag>
  constexpr static const auto& get_value_for_comparison(const Tag& tag,
                                                        const TS& ts) {
    return tag(ts);
  }
  TagOrValue1 tag_or_value1;
  TagOrValue2 tag_or_value2;
  template <typename TS>
  bool operator()(const TS& ts) const {
    const auto& a = get_value_for_comparison(tag_or_value1, ts);
    const auto& b = get_value_for_comparison(tag_or_value2, ts);
    if constexpr (comparison == tag_comparison::eq) {
      return a == b;
    }
    if constexpr (comparison == tag_comparison::ne) {
      return a != b;
    }
    if constexpr (comparison == tag_comparison::lt) {
      return a < b;
    }
    if constexpr (comparison == tag_comparison::gt) {
      return a > b;
    }
    if constexpr (comparison == tag_comparison::le) {
      return a <= b;
    }
    if constexpr (comparison == tag_comparison::ge) {
      return a >= b;
    }
  }
};

template <tag_comparison comparison, typename T1, typename T2>
constexpr auto make_tag_comparator_predicate(T1 a, T2 b) {
  return tag_comparator_predicate<T1, T2, comparison>{std::move(a),
                                                      std::move(b)};
}

namespace tag_relops {
// Compare two tags
template <typename A, typename B>
requires is_tuple_tag_v<A> || is_tuple_tag_v<B>
constexpr auto operator==(A a, B b) {
  return make_tag_comparator_predicate<tag_comparison::eq>(a, b);
}

template <typename A, typename B>
requires is_tuple_tag_v<A> || is_tuple_tag_v<B>
constexpr auto operator!=(A a, B b) {
  return make_tag_comparator_predicate<tag_comparison::ne>(a, b);
}

template <typename A, typename B>
requires is_tuple_tag_v<A> || is_tuple_tag_v<B>
constexpr auto operator<=(A a, B b) {
  return make_tag_comparator_predicate<tag_comparison::le>(a, b);
}

template <typename A, typename B>
requires is_tuple_tag_v<A> || is_tuple_tag_v<B>
constexpr auto operator>=(A a, B b) {
  return make_tag_comparator_predicate<tag_comparison::ge>(a, b);
}

template <typename A, typename B>
requires is_tuple_tag_v<A> || is_tuple_tag_v<B>
constexpr auto operator<(A a, B b) {
  return make_tag_comparator_predicate<tag_comparison::lt>(a, b);
}

template <typename A, typename B>
requires is_tuple_tag_v<A> || is_tuple_tag_v<B>
constexpr auto operator>(A a, B b) {
  return make_tag_comparator_predicate<tag_comparison::gt>(a, b);
}

}  // namespace tag_relops

}  // namespace internal_tagged_tuple

using internal_tagged_tuple::auto_;
using internal_tagged_tuple::get;
using internal_tagged_tuple::member;
using internal_tagged_tuple::tag;
using internal_tagged_tuple::tagged_tuple;
using internal_tagged_tuple::tagged_tuple_init_v;
using internal_tagged_tuple::tagged_tuple_ref_t;
using internal_tagged_tuple::tagged_tuple_value_type_t;

namespace tag_relops = internal_tagged_tuple::tag_relops;

namespace literals {
template <
    internal_tagged_tuple::fixed_string fs>  // placeholder type for deduction
constexpr auto operator""_tag() {
  return tag<fs>;
}
}  // namespace literals

}  // namespace ftsd
