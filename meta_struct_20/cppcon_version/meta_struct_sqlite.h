// Copyright 2020 Google LLC
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
#pragma once
#include <assert.h>
#include <sqlite3.h>

#include <array>
#include <cstdint>
#include <exception>
#include <iostream>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>

#include "meta_struct.h"

namespace ftsd {

namespace sqlite_experimental {

template <typename T, typename... GoodValues>
void check_sqlite_return(T r, GoodValues... good_values) {
  bool success = false;
  if constexpr (sizeof...(good_values) == 0) {
    success = (r == SQLITE_OK);
  } else {
    success = ((r == good_values) || ...);
  }
  if (!success) {
    std::string err_msg = sqlite3_errstr(r);
    assert(success);
    std::cerr << "SQLITE ERROR " << r << " " << err_msg;
    throw std::runtime_error("sqlite error: " + err_msg);
  }
}

inline bool read_row_into(sqlite3_stmt *stmt, int index, std::int64_t &v) {
  auto type = sqlite3_column_type(stmt, index);
  if (type == SQLITE_INTEGER) {
    v = sqlite3_column_int64(stmt, index);
    return true;
  } else if (type == SQLITE_NULL) {
    return false;
  } else {
    return false;
  }
}

inline bool read_row_into(sqlite3_stmt *stmt, int index, double &v) {
  auto type = sqlite3_column_type(stmt, index);
  if (type == SQLITE_FLOAT) {
    v = sqlite3_column_double(stmt, index);
    return true;
  } else if (type == SQLITE_NULL) {
    return false;
  } else {
    return false;
  }
}

inline bool read_row_into(sqlite3_stmt *stmt, int index, std::string_view &v) {
  auto type = sqlite3_column_type(stmt, index);
  if (type == SQLITE_TEXT) {
    const char *ptr =
        reinterpret_cast<const char *>(sqlite3_column_text(stmt, index));
    auto size = sqlite3_column_bytes(stmt, index);

    v = std::string_view(ptr, ptr ? size : 0);
    return true;
  } else if (type == SQLITE_NULL) {
    return false;
  } else {
    return false;
  }
}

template <typename T>
inline bool read_row_into(sqlite3_stmt *stmt, int index, std::optional<T> &v) {
  auto type = sqlite3_column_type(stmt, index);
  if (type == SQLITE_NULL) {
    v = std::nullopt;
    return true;
  } else {
    v.emplace();
    return read_row_into(stmt, index, *v);
  }
}

template <typename RowType>
auto read_row(sqlite3_stmt *stmt) {
  RowType row = {};
  std::size_t count = sqlite3_column_count(stmt);

  auto size = meta_struct_size(row);
  assert(size == count);
  if (size != count) {
    throw std::runtime_error(
        "sqlite error: mismatch between read_row and sql columns");
  }
  int index = 0;
  auto read_into = [&](auto &m) mutable {
    read_row_into(stmt, index, m.value);
    ++index;
  };

  meta_struct_apply([&](auto &...m) mutable { (read_into(m), ...); }, row);
  return row;
}

template <typename RowType>
struct row_range {
  RowType row;
  int last_result = 0;
  sqlite3_stmt *stmt;

  row_range(sqlite3_stmt *stmt) : stmt(stmt) { next(); }

  struct end_type {};

  end_type end() { return {}; }

  void next() {
    last_result = sqlite3_step(stmt);
    check_sqlite_return(last_result, SQLITE_DONE, SQLITE_ROW);
  }

  struct row_iterator {
    row_range *p;

    row_iterator &operator++() {
      p->next();
      return *this;
    }

    bool operator!=(end_type) { return p->last_result == SQLITE_ROW; }
    bool operator==(end_type) { return p->last_result != SQLITE_ROW; }

    RowType &operator*() {
      p->row = read_row<RowType>(p->stmt);
      return p->row;
    }
  };

  bool has_error() const {
    return last_result != SQLITE_ROW && last_result != SQLITE_DONE;
  }

  row_iterator begin() { return {this}; }
};

inline bool bind_impl(sqlite3_stmt *stmt, int index, double v) {
  auto r = sqlite3_bind_double(stmt, index, v);
  return r == SQLITE_OK;
}

inline bool bind_impl(sqlite3_stmt *stmt, int index, std::int64_t v) {
  auto r = sqlite3_bind_int64(stmt, index, v);
  return r == SQLITE_OK;
}

inline bool bind_impl(sqlite3_stmt *stmt, int index, std::string_view v) {
  auto r = sqlite3_bind_text(stmt, index, v.data(), static_cast<int>(v.size()),
                             SQLITE_TRANSIENT);
  return r == SQLITE_OK;
}

template <typename T>
bool bind_impl(sqlite3_stmt *stmt, int index, const std::optional<T> &v) {
  if (v.has_value()) {
    return bind_impl(stmt, index, *v);
  } else {
    auto r = sqlite3_bind_null(stmt, index);
    return r == SQLITE_OK;
  }
}

auto to_concrete(const std::string_view &v) { return std::string(v); }
auto to_concrete(std::int64_t i) { return i; }
auto to_concrete(double d) { return d; }
template <typename T>
auto to_concrete(const std::optional<T> &o)
    -> std::optional<decltype(to_concrete(std::declval<T>()))> {
  if (!o) {
    return std::nullopt;
  } else {
    return to_concrete(*o);
  }
}

template <typename T>
auto to_concrete(std::optional<T> &&o)
    -> std::optional<decltype(to_concrete(std::declval<T>()))> {
  if (!o) {
    return std::nullopt;
  } else {
    return to_concrete(std::move(*o));
  }
}

template <auto... Tags, typename... Ts, auto... Init, auto... Attributes>
auto to_concrete(const meta_struct<member<Tags, Ts, Init, Attributes>...> &t) {
  return meta_struct_apply(
      [](auto &...m) { return meta_struct{(arg<m.tag()> = m.value)...}; }, t);
}

template <auto... Tags, typename... Ts, auto... Init, auto... Attributes>
auto to_concrete(meta_struct<ftsd::member<Tags, Ts, Init, Attributes>...> &&t) {
  return meta_struct{(arg<Tags> = to_concrete(get<Tags>(std::move(t))))...};
}

template <bool make_optional, typename Member>
auto maybe_make_optional(Member m) {
  if constexpr (make_optional) {
    return member<Member::tag(), std::optional<typename Member::element_type>,
                  Member::init(), Member::attributes()>{
        m, arg<Member::tag()> = std::optional<typename Member::element_type>()};
  } else {
    return m;
  }
}

template <fixed_string Tag>
struct string_to_type;

template <>
struct string_to_type<"integer"> {
  using type = std::int64_t;
};

template <>
struct string_to_type<"text"> {
  using type = std::string_view;
};

template <>
struct string_to_type<"real"> {
  using type = double;
};

template <fixed_string Tag>
using string_to_type_t = typename string_to_type<Tag>::type;

constexpr std::string_view start_group = "{{";
constexpr std::string_view end_group = "}}";

constexpr std::string_view delimiters = ", ();";
constexpr std::string_view quotes = "\"\'";

struct type_specs_count {
  std::size_t fields;
  std::size_t params;
  auto operator<=>(const type_specs_count &) const = default;
};

inline constexpr std::string_view start_comment = "/*:";
inline constexpr std::string_view end_comment = "*/";

template <fixed_string query_string>
constexpr auto get_type_spec_count() {
  constexpr auto sv = query_string.sv();
  type_specs_count count{0, 0};

  std::string_view str = sv;
  while (!str.empty()) {
    auto pos = str.find(start_comment);
    if (pos == str.npos) break;
    pos += start_comment.size();
    str = str.substr(pos);
    pos = str.find(end_comment);
    if (pos == str.npos) {
      break;
    }
    auto comment = str.substr(0, pos);
    if (comment.find(":") == comment.npos) {
      ++count.fields;
    } else {
      ++count.params;
    }
  }
  return count;
}

template <typename First, typename Second>
struct pair {
  First first;
  Second second;
  constexpr auto operator<=>(const pair &other) const = default;
};

struct type_spec {
  pair<std::size_t, std::size_t> name;
  pair<std::size_t, std::size_t> type;
  bool optional;
  constexpr auto operator<=>(const type_spec &other) const = default;
};

template <std::size_t N>
struct type_specs {
  auto operator<=>(const type_specs &) const = default;
  type_spec data[N];
  static constexpr std::size_t size() { return N; }
  constexpr auto &operator[](std::size_t i) const { return data[i]; }
  constexpr auto &operator[](std::size_t i) { return data[i]; }
};

template <std::size_t Fields, std::size_t Params>
struct combined_type_specs {
  type_specs<Fields> fields;
  type_specs<Params> params;
  auto operator<=>(const combined_type_specs &) const = default;
};

template <>
struct type_specs<0> {
  auto operator<=>(const type_specs &) const = default;
  static constexpr std::size_t size() { return 0; }
};

template <fixed_string query_string>
constexpr auto parse_type_specs() {
  constexpr auto sv = query_string.sv();
  constexpr auto ret_counts = get_type_spec_count<query_string>();
  combined_type_specs<ret_counts.fields, ret_counts.params> ret = {};

  type_specs_count counts{0, 0};

  auto in_range_inclusive = [](char c, char b, char e) {
    return b <= c && c <= e;
  };
  auto is_name = [in_range_inclusive](char c) {
    return in_range_inclusive(c, 'A', 'Z') || in_range_inclusive(c, 'a', 'z') ||
           in_range_inclusive(c, '0', '9') || c == '_' || c == '.';
  };

  auto is_space = [](char c) {
    return c == ' ' || c == '\n' || c == '\r' || c == '\t';
  };

  const std::string_view str = sv;
  std::size_t offset = 0;
  while (offset != str.npos && offset < str.size()) {
    auto pos = str.find(start_comment, offset);
    offset = pos + start_comment.size();
    if (pos == str.npos) break;
    auto end_pos = str.find(end_comment, offset);
    auto comment_begin = pos + start_comment.size();
    auto comment_end = end_pos;
    auto comment = str.substr(comment_begin, comment_end - comment_begin);
    auto colon_pos = comment.find(":");
    if constexpr (ret_counts.fields > 0) {
      if (colon_pos == comment.npos) {
        int prev_name_end = static_cast<int>(pos + 1);
        int prev_name_begin = 0;
        for (int rpos = static_cast<int>(pos - 1); rpos > -1; --rpos) {
          char c = str[rpos];
          if (is_name(c)) {
            if (prev_name_end == pos + 1) {
              prev_name_end = rpos + 1;
            }
          } else {
            if (prev_name_end == pos + 1) continue;
            prev_name_begin = rpos + 1;
            break;
          }
        }
        type_spec &spec = ret.fields[counts.fields];
        spec.name.first = prev_name_begin;
        spec.name.second = prev_name_end - prev_name_begin;
        spec.type.first = comment_begin;
        spec.type.second = comment_end - comment_begin;
        bool optional = false;
        auto type = str.substr(comment_begin, comment_end - comment_begin);
        if (!type.empty() && type.ends_with("?")) {
          optional = true;
          --spec.type.second;
        }
        spec.optional = optional;
        auto name =
            str.substr(spec.name.first, spec.name.second - spec.name.first);
        ++counts.fields;
      }
    }

    if constexpr (ret_counts.params > 0) {
      if (colon_pos != comment.npos) {
        auto name = comment.substr(0, colon_pos);
        auto type = comment.substr(colon_pos + 1);
        bool optional = false;
        if (!type.empty() && type.ends_with("?")) {
          optional = true;
          type.remove_suffix(1);
        }
        type_spec &spec = ret.params[counts.params];
        spec.name.first = comment_begin;
        spec.name.second = name.size();
        spec.type.first = comment_begin + colon_pos + 1;
        spec.type.second = type.size();
        spec.optional = optional;
        ++counts.params;
      }
    }
  }
  return ret;
}

template <fixed_string query_string, type_spec ts>
constexpr auto make_member_ts() {
  constexpr auto sv = query_string.sv();
  constexpr auto name = fixed_string<ts.name.second>::from_string_view(
      sv.substr(ts.name.first, ts.name.second));
  constexpr auto type = fixed_string<ts.type.second>::from_string_view(
      sv.substr(ts.type.first, ts.type.second));
  return maybe_make_optional<ts.optional>(arg<name> =
                                              (string_to_type_t<type>{}));
}

template <fixed_string query_string, type_specs ts, std::size_t... I>
constexpr auto make_members_helper(std::index_sequence<I...>) {
  return meta_struct{make_member_ts<query_string, ts[I]>()...};
}

template <fixed_string query_string>
constexpr auto make_members() {
  constexpr auto ts = parse_type_specs<query_string>();
  constexpr auto fields = ts.fields;
  if constexpr (fields.size() == 0) {
    return meta_struct<>{};
  } else {
    return make_members_helper<query_string, fields>(
        std::make_index_sequence<fields.size()>());
  }
}

template <fixed_string query_string>
constexpr auto make_parameters() {
  constexpr auto ts = parse_type_specs<query_string>();
  return make_members_helper<query_string, ts.params>(
      std::make_index_sequence<ts.params.size()>());
}

template <typename T>
std::true_type is_optional(const std::optional<T> &);

std::false_type is_optional(...);

template <typename PTuple>
void do_binding(sqlite3_stmt *stmt, PTuple p_tuple) {
  int index = 1;
  meta_struct_for_each([&](auto &m) mutable {
    auto r = bind_impl(stmt, index, m.value);
    check_sqlite_return<bool>(r, true);
    ++index;
  },p_tuple);
}

struct stmt_closer {
  void operator()(sqlite3_stmt *s) {
    if (s) sqlite3_finalize(s);
  }
};

using unique_stmt = std::unique_ptr<sqlite3_stmt, stmt_closer>;

template <fixed_string Query>
class prepared_statement {
  using RowType = decltype(make_members<Query>());
  using PTuple = decltype(make_parameters<Query>());

  unique_stmt stmt_;
  void reset_stmt() {
    auto r = sqlite3_reset(stmt_.get());
    check_sqlite_return(r);
    r = sqlite3_clear_bindings(stmt_.get());
    check_sqlite_return(r);
  }

 public:
  prepared_statement(sqlite3 *sqldb) {
    auto sv = Query.sv();
    sqlite3_stmt *stmt;
    auto specs = parse_type_specs<Query>();
    auto rc = sqlite3_prepare_v2(sqldb, sv.data(), static_cast<int>(sv.size()),
                                 &stmt, 0);
    check_sqlite_return(rc);
    stmt_.reset(stmt);
  }
  row_range<RowType> execute_rows() requires(meta_struct_size<PTuple>() == 0) {
    reset_stmt();
    return row_range<RowType>(stmt_.get());
  }
  row_range<RowType> execute_rows(PTuple p_tuple) {
    reset_stmt();
    do_binding(stmt_.get(), std::move(p_tuple));
    return row_range<RowType>(stmt_.get());
  }
  std::optional<decltype(to_concrete(std::declval<RowType>()))>
  execute_single_row(PTuple p_tuple) {
    auto rng = execute_rows(std::move(p_tuple));
    auto begin = rng.begin();
    if (begin != rng.end()) {
      return to_concrete(*begin);
    } else {
      return std::nullopt;
    }
  }
  std::optional<decltype(to_concrete(std::declval<RowType>()))>
  execute_single_row() requires(meta_struct_size<PTuple>() == 0) {
    auto rng = execute_rows();
    auto begin = rng.begin();
    if (begin != rng.end()) {
      return to_concrete(*begin);
    } else {
      return std::nullopt;
    }
  }
  void execute(PTuple p_tuple) {
    reset_stmt();
    do_binding(stmt_.get(), std::move(p_tuple));
    auto r = sqlite3_step(stmt_.get());
    check_sqlite_return(r, SQLITE_DONE);
  }

  void execute() requires(meta_struct_size<PTuple>() == 0) {
    reset_stmt();
    auto r = sqlite3_step(stmt_.get());
    check_sqlite_return(r, SQLITE_DONE);
  }
};

template <fixed_string S, typename T>
decltype(auto) field(T &&t) {
  return ftsd::get<S>(std::forward<T>(t));
}

template <fixed_string fs, typename T>
auto bind(T &&t) {
  return arg<fs> = std::forward<T>(t);
}

template <typename Tag>
struct param_helper {
  template <typename T>
  auto operator=(T t) {
    return make_member<Tag, T>(std::move(t));
  }
};

}  // namespace sqlite_experimental

using sqlite_experimental::bind;
using sqlite_experimental::field;
using sqlite_experimental::prepared_statement;
using sqlite_experimental::to_concrete;

}  // namespace ftsd
