#include <algorithm>
#include <cstddef>
#include <optional>
#include <string_view>
#include <type_traits>

template <std::size_t N>
struct fixed_string {
  constexpr fixed_string(const char (&foo)[N + 1]) {
    std::copy_n(foo, N + 1, data);
  }
  constexpr fixed_string(std::string_view s) {
    static_assert(s.size() <= N);
    std::copy(s.begin(), s.end(), data);
  }
  constexpr std::string_view sv() const { return std::string_view(data); }
  auto operator<=>(const fixed_string&) const = default;
  char data[N + 1] = {};
};
template <std::size_t N>
fixed_string(const char (&str)[N]) -> fixed_string<N - 1>;

template <fixed_string Tag, typename T>
struct tag_and_value {
  T value;
};

struct no_conversion {};
template <typename... TagsAndValues>
struct parms : TagsAndValues... {
  constexpr operator no_conversion() const { return no_conversion{}; }
};

template <typename... TagsAndValues>
parms(TagsAndValues...) -> parms<TagsAndValues...>;

template <fixed_string Tag>
struct arg_type {
  template <typename T>
  constexpr auto operator=(T t) const {
    return tag_and_value<Tag, T>{std::move(t)};
  }
};

template <fixed_string Tag>
inline constexpr auto arg = arg_type<Tag>{};

template <typename T>
struct default_init {
  constexpr default_init() = default;
  auto operator<=>(const default_init&) const = default;
  constexpr auto operator()() const {
    if constexpr (std::is_default_constructible_v<T>) {
      return T{};
    }
  }
};

template <typename T, typename Self, typename F>
auto call_init(Self&, F& f) requires(requires {
  { f() } -> std::convertible_to<T>;
}) {
  return f();
}

template <typename T, typename Self, typename F>
auto call_init(Self& self, F& f) requires(requires {
  { f(self) } -> std::convertible_to<T>;
}) {
  return f(self);
}

template <typename T, typename Self, typename F>
auto call_init(Self& self, F& f) requires(requires {
  { f() } -> std::same_as<void>;
}) {
  static_assert(!std::is_same_v<decltype(f()), void>,
                "Required argument not specified");
}

inline constexpr auto required = [] {};

template <fixed_string Tag, typename T, auto Init = default_init<T>()>
struct member {
  constexpr static auto tag() { return Tag; }
  constexpr static auto init() { return Init; }
  using element_type = T;
  T value;
  template <typename Self, typename OtherT>
  constexpr member(Self&, tag_and_value<Tag, OtherT> tv)
      : value(std::move(tv.value)) {}

  template <typename Self>
  constexpr member(Self& self, no_conversion)
      : value(call_init<T>(self, Init)) {}
  template <typename Self>
  constexpr member(Self& self,
                   tag_and_value<Tag, std::optional<std::remove_reference_t<T>>>
                       tv_or) requires(!std::is_reference_v<T>)
      : value(tv_or.value.has_value() ? std::move(*tv_or.value)
                                      : call_init<T>(self, Init)) {}

  template <typename Self, typename OtherT, auto OtherInit>
  requires(std::is_convertible_v<OtherT, T>)
      member(Self&, const member<Tag, OtherT, OtherInit>& other)
      : value(other.value) {}

  template <typename Self, typename OtherT, auto OtherInit>
  requires(std::is_convertible_v<OtherT, T>)
      member(Self&, member<Tag, OtherT, OtherInit>&& other)
      : value(std::move(other.value)) {}

  template <typename OtherT, auto OtherInit>
  requires(std::is_convertible_v<OtherT, T>) member& operator=(
      const member<Tag, OtherT, OtherInit>& other) {
    value = other.value;
    return *this;
  }

  template <typename OtherT, auto OtherInit>
  requires(std::is_convertible_v<OtherT, T>) member& operator=(
      member<Tag, OtherT, OtherInit>&& other) {
    value = std::move(other.value);
    return *this;
  }

  constexpr member(member&&) = default;
  constexpr member(const member&) = default;

  constexpr member& operator=(member&&) = default;
  constexpr member& operator=(const member&) = default;

  auto operator<=>(const member&) const = default;
};

template <typename... Members>
struct meta_struct_impl : Members... {
  template <typename... Args>
  constexpr meta_struct_impl(parms<Args...> p)
      : Members(*this, std::move(p))... {}

  constexpr meta_struct_impl() : Members(*this, no_conversion{})... {}
  constexpr meta_struct_impl(meta_struct_impl&&) = default;
  constexpr meta_struct_impl(const meta_struct_impl&) = default;
  constexpr meta_struct_impl& operator=(meta_struct_impl&&) = default;
  constexpr meta_struct_impl& operator=(const meta_struct_impl&) = default;

  template <typename... OtherMembers>
  constexpr meta_struct_impl(meta_struct_impl<OtherMembers...>&& other)
      : Members(*this, std::move(other))... {};
  template <typename... OtherMembers>
  constexpr meta_struct_impl(const meta_struct_impl<OtherMembers...>& other)
      : Members(*this, other)... {};

  template <typename... OtherMembers>
  constexpr meta_struct_impl& operator=(
      meta_struct_impl<OtherMembers...>&& other) {
    ((static_cast<Members&>(*this) = std::move(other)), ...);
    return *this;
  };

  template <typename... OtherMembers>
  constexpr meta_struct_impl& operator=(
      const meta_struct_impl<OtherMembers...>& other) {
    ((static_cast<Members&>(*this) = other), ...);
    return *this;
  };

  constexpr operator no_conversion() const { return no_conversion{}; }

  auto operator<=>(const meta_struct_impl&) const = default;
};

template <typename... Members>
struct meta_struct : meta_struct_impl<Members...> {
  using super = meta_struct_impl<Members...>;
  template <typename... TagsAndValues>
  constexpr meta_struct(TagsAndValues... tags_and_values)
      : super(parms(std::move(tags_and_values)...)) {}

  constexpr meta_struct() = default;
  constexpr meta_struct(meta_struct&&) = default;
  constexpr meta_struct(const meta_struct&) = default;
  constexpr meta_struct& operator=(meta_struct&&) = default;
  constexpr meta_struct& operator=(const meta_struct&) = default;

  template <typename... OtherMembers>
  constexpr meta_struct(meta_struct<OtherMembers...>&& other)
      : super(std::move(other)) {}

  template <typename... OtherMembers>
  constexpr meta_struct(const meta_struct<OtherMembers...>& other)
      : super(other) {}

  template <typename... OtherMembers>
  constexpr meta_struct& operator=(meta_struct<OtherMembers...>&& other) {
    static_cast<super&>(*this) = std::move(other);
    return *this;
  }

  template <typename... OtherMembers>
  constexpr meta_struct& operator=(const meta_struct<OtherMembers...>& other) {
    static_cast<super&>(*this) = other;
    return *this;
  }

  auto operator<=>(const meta_struct&) const = default;
};

template <typename F, typename... MembersImpl>
constexpr decltype(auto) meta_struct_apply(
    F&& f, meta_struct_impl<MembersImpl...>& m) {
  return std::forward<F>(f)(static_cast<MembersImpl&>(m)...);
}

template <typename F, typename... MembersImpl>
constexpr decltype(auto) meta_struct_apply(
    F&& f, const meta_struct_impl<MembersImpl...>& m) {
  return std::forward<F>(f)(static_cast<const MembersImpl&>(m)...);
}

template <typename F, typename... MembersImpl>
constexpr decltype(auto) meta_struct_apply(
    F&& f, meta_struct_impl<MembersImpl...>&& m) {
  return std::forward<F>(f)(static_cast<MembersImpl&&>(m)...);
}

template <typename MetaStructImpl>
struct apply_static_impl;

template <typename... MembersImpl>
struct apply_static_impl<meta_struct_impl<MembersImpl...>> {
  template <typename F>
  constexpr static decltype(auto) apply(F&& f) {
    return f(static_cast<MembersImpl*>(nullptr)...);
  }
};

template <typename MetaStruct, typename F>
auto meta_struct_apply(F&& f) {
  return apply_static_impl<typename MetaStruct::super>::apply(
      std::forward<F>(f));
}

template <fixed_string tag, typename T, auto Init>
decltype(auto) get_impl(member<tag, T, Init>& m) {
  return (m.value);
}

template <fixed_string tag, typename T, auto Init>
decltype(auto) get_impl(const member<tag, T, Init>& m) {
  return (m.value);
}

template <fixed_string tag, typename T, auto Init>
decltype(auto) get_impl(member<tag, T, Init>&& m) {
  return std::move(m.value);
}

template <fixed_string tag, typename MetaStruct>
decltype(auto) get(MetaStruct&& s) {
  return get_impl<tag>(std::forward<MetaStruct>(s));
}

#include <iostream>
#include <string>

template <typename MetaStruct>
void print(std::ostream& os, const MetaStruct& ms) {
  meta_struct_apply(
      [&](const auto&... m) {
        auto print_item = [&](auto& m) {
          std::cout << m.tag().sv() << ":" << m.value << "\n";
        };
        (print_item(m), ...);
      },
      ms);
};

using NameAndIdArgs = meta_struct<     //
    member<"name", std::string_view>,  //
    member<"id", const int&>           //
    >;

void print_name_id(NameAndIdArgs args) {
  std::cout << "Name is " << get<"name">(args) << " and id is "
            << get<"id">(args) << "\n";
}

int main() {
  using Person = meta_struct<                                               //
      member<"id", int, required>,                                          //
      member<"name", std::string, required>,                                //
      member<"score", int, [](auto& self) { return get<"id">(self) + 1; }>  //
      >;

  auto print = [](auto& t) {
    meta_struct_apply(
        [&](const auto&... m) {
          ((std::cout << m.tag().sv() << ":" << m.value << "\n"), ...);
        },
        t);
  };

  Person p{arg<"id"> = 2, arg<"name"> = "John"};

  using NameAndId = meta_struct<         //
      member<"name", std::string_view>,  //
      member<"id", int>                  //
      >;

  print(p);
  std::cout << "\n";
  NameAndId n = p;
  print(n);
  std::cout << "\n";

  print_name_id(p);
  print_name_id(n);
}
