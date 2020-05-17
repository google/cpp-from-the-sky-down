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

namespace skydown {

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

inline bool read_row_into(sqlite3_stmt *stmt, int index,
                          std::optional<std::int64_t> &v) {
  auto type = sqlite3_column_type(stmt, index);
  if (type == SQLITE_INTEGER) {
    v = sqlite3_column_int64(stmt, index);
    return true;
  } else if (type == SQLITE_NULL) {
    v = std::nullopt;
    return true;
  } else {
    return false;
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

inline bool read_row_into(sqlite3_stmt *stmt, int index,
                          std::optional<double> &v) {
  auto type = sqlite3_column_type(stmt, index);
  if (type == SQLITE_FLOAT) {
    v = sqlite3_column_double(stmt, index);
    return true;
  } else if (type == SQLITE_NULL) {
    v = std::nullopt;
    return true;
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

inline bool read_row_into(sqlite3_stmt *stmt, int index,
                          std::optional<std::string_view> &v) {
  auto type = sqlite3_column_type(stmt, index);
  if (type == SQLITE_TEXT) {
    const char *ptr =
        reinterpret_cast<const char *>(sqlite3_column_text(stmt, index));
    auto size = sqlite3_column_bytes(stmt, index);

    v = std::string_view(ptr, ptr ? size : 0);
    return true;
  } else if (type == SQLITE_NULL) {
    v = std::nullopt;
    return true;
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

template <typename RowType>
auto read_row(sqlite3_stmt *stmt) {
  RowType row = {};
  std::size_t count = sqlite3_column_count(stmt);

  auto size = tuple_size(row);
  assert(size == count);
  if (size != count) {
    throw std::runtime_error(
        "sqlite error: mismatch between read_row and sql columns");
  }
  int index = 0;
  for_each(row, [&](auto &m) mutable {
    read_row_into(stmt, index, m.value);
    ++index;
  });
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

template <typename Tag, typename T>
struct member {
  T value;
  using tag_type = Tag;
  using value_type = T;
};

template <typename Tag, typename T>
auto make_member(T t) {
  return member<Tag, T>{std::move(t)};
}

template <typename... Members>
struct tagged_tuple : Members... {};

template <typename... Members>
tagged_tuple(Members &&...) -> tagged_tuple<std::decay_t<Members>...>;

template <typename... Members>
constexpr auto tuple_size(const tagged_tuple<Members...> &) {
  return sizeof...(Members);
}

template <typename... Members, typename F>
void for_each(const tagged_tuple<Members...> &m, F f) {
  (f(static_cast<const Members &>(m)), ...);
}

template <typename... Members, typename F>
void for_each(tagged_tuple<Members...> &m, F f) {
  (f(static_cast<Members &>(m)), ...);
}

template <typename Tag, typename T>
decltype(auto) get(member<Tag, T> &m) {
  return (m.value);
}

template <typename Tag, typename T>
decltype(auto) get(const member<Tag, T> &m) {
  return (m.value);
}

template <typename Tag, typename T>
decltype(auto) get(member<Tag, T> &&m) {
  return std::move(m.value);
}

template <typename Tag, typename T>
decltype(auto) get(const member<Tag, T> &&m) {
  return std::move(m.value);
}

template <std::size_t N>
struct fixed_string {
  constexpr fixed_string(const char (&foo)[N + 1]) {
    std::copy_n(foo, N + 1, data);
  }
  constexpr fixed_string(const fixed_string &) = default;
  constexpr fixed_string(std::string_view s){

    std::copy_n(s.data(), N, data);

      
      };
  auto operator<=>(const fixed_string &) const = default;
  char data[N + 1];
  constexpr std::string_view sv() const {
    return std::string_view(&data[0], N);
  }
  constexpr auto size() const { return N; }
  constexpr auto operator[](std::size_t i) const { return data[i]; }
};

template <fixed_string fs>
struct compile_string {};

template <std::size_t N>
fixed_string(const char (&str)[N]) -> fixed_string<N - 1>;

template <std::size_t N>
fixed_string(fixed_string<N>) -> fixed_string<N>;

template <bool make_optional, typename Tag, typename T>
auto maybe_make_optional(member<Tag, T> m) {
  if constexpr (make_optional) {
    return member<Tag, std::optional<T>>{};
  } else {
    return m;
  }
}

template <typename>
struct string_to_type;

template <>
struct string_to_type<compile_string<"int">> {
  using type = std::int64_t;
};

template <>
struct string_to_type<compile_string<"string">> {
  using type = std::string_view;
};

template <>
struct string_to_type<compile_string<"double">> {
  using type = double;
};

template <typename T>
using string_to_type_t = typename string_to_type<T>::type;

constexpr std::string_view start_group = "{{";
constexpr std::string_view end_group = "}}";

template <fixed_string parm, bool is_parms>
constexpr auto get_type_spec_count(std::string_view start_group,
                                   std::string_view end_group) {
  constexpr auto sv = parm.sv();
  std::size_t count = 0;
  for (std::size_t i = sv.find(start_group); i != std::string_view::npos;) {
    if (is_parms == (sv[i + start_group.size()] == '?')) {
      ++count;
    }
    i = sv.find(start_group, i + 1);
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
  pair<int, int> name = {-1, -1};
  pair<int, int> type = {-1, -1};
  bool optional;
  constexpr auto operator<=>(const type_spec &other) const = default;
};

template <std::size_t N>
struct type_specs {
  auto operator<=>(const type_specs &) const = default;
  type_spec data[N];
  static constexpr std::size_t size() { return N; }
  constexpr auto& operator[](std::size_t i) const { return data[i]; }
  constexpr auto& operator[](std::size_t i) { return data[i]; }
};

constexpr type_spec parse_type_spec(std::size_t base,std::string_view sv) {
  auto colon = sv.find(":");
  auto name = sv.substr(0, colon);

  bool optional = sv[sv.size() - 1] == '?' ? true : false;
  auto last_colon = sv.find_last_of(':');
  auto size = sv.size();
  auto new_size = size - (optional ? 2 : 1);
  std::string_view type = sv.substr(last_colon + 1, new_size - last_colon);
  return type_spec{.name = {base, name.size()},
                   .type = {base + last_colon + 1, type.size()},
                   .optional = optional};
}

template <fixed_string parm, bool is_parms>
constexpr auto parse_type_specs() {
  constexpr auto sv = parm.sv();
  constexpr auto size =
      get_type_spec_count<parm, is_parms>(start_group, end_group);
  type_specs<size> ar = {};
  std::size_t count = 0;
  for (std::size_t i = sv.find(start_group); i != std::string_view::npos;) {
    auto end = sv.find(end_group, i);
    auto start = i + start_group.size();
    if (is_parms == (sv[start] == '?')) {
      if (is_parms) ++start;
      ar[count] = parse_type_spec(start,sv.substr(start, end - start));
      ++count;
    }
    i = sv.find(start_group, i + 1);
  }

  return ar;
}

template <fixed_string fs, type_spec ts>
constexpr auto make_member_ts() {
  constexpr auto sv = fs.sv();
  static_assert(sv.size() != 0, "string view conversion");
  static_assert(ts.name.second != 0);
  constexpr fixed_string<ts.name.second> name{
      sv.substr(ts.name.first, ts.name.second)};
  constexpr fixed_string<ts.type.second> type =
      sv.substr(ts.type.first, ts.type.second);
  return maybe_make_optional<ts.optional>(make_member<compile_string<name>>(
      string_to_type_t<compile_string<type>>{}));
}

template <fixed_string parm, type_specs ts, std::size_t... I>
constexpr auto make_members_helper(std::index_sequence<I...>) {
  return tagged_tuple{make_member_ts<parm, ts[I]>()...};
}

template <fixed_string parm>
constexpr auto make_members() {
  constexpr auto ts = parse_type_specs<parm, false>();

  if constexpr (ts.size()) static_assert(ts[ts.size() - 1].name.second != -1,"make members afiled");
  return make_members_helper<parm, ts>(std::make_index_sequence<ts.size()>());
}

template <fixed_string parm>
constexpr auto make_parameters() {
  constexpr auto ts = parse_type_specs<parm, true>();
  if constexpr (ts.size()) static_assert(ts[ts.size() - 1].name.second != -1);
  return make_members_helper<parm, ts>(std::make_index_sequence<ts.size()>());
}

// todo make constexpr
inline std::string get_sql_string(std::string_view sv,
                                  std::string_view start_group,
                                  std::string_view end_group) {
  std::string ret;
  ret.reserve(sv.size());
  std::size_t prev_i = 0;
  for (std::size_t i = sv.find(start_group); i != std::string_view::npos;) {
    ret += std::string(sv.substr(prev_i, i - prev_i));
    auto end = sv.find(end_group, i);

    ret += " ";
    auto ts_str =
        sv.substr(i + start_group.size(), end - (i + start_group.size()) - 1);
    auto ts = parse_type_spec(i + start_group.size(),ts_str);
    auto name = sv.substr(ts.name.first, ts.name.second);
    if (name.front() != '?') {
      ret += std::string(name);
    } else {
      ret += '?';
    }
    ret += " ";
    prev_i = end + end_group.size();
    i = sv.find(start_group, i + 1);
  }
  ret += std::string(sv.substr(prev_i));

  return ret;
}

template <typename PTuple, typename ATuple>
void do_binding(sqlite3_stmt *stmt, PTuple p_tuple, ATuple a_tuple) {
  int index = 1;
  skydown::sqlite_experimental::for_each(p_tuple, [&](auto &m) mutable {
    using m_t = std::decay_t<decltype(m)>;
    using tag = typename m_t::tag_type;
    m.value = std::move(get<tag>(a_tuple));
    auto r = bind_impl(stmt, index, m.value);
    check_sqlite_return<bool>(r, true);
    ++index;
  });
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
    auto query_string = get_sql_string(sv, start_group, end_group);
    auto rc =
        sqlite3_prepare_v2(sqldb, query_string.c_str(),
                           static_cast<int>(query_string.size()), &stmt, 0);
    check_sqlite_return(rc);
    stmt_.reset(stmt);
  }
  template <typename... Args>
  row_range<RowType> execute_rows(Args &&... args) {
    reset_stmt();

    PTuple p_tuple = {};
    tagged_tuple a_tuple{std::forward<Args>(args)...};
    do_binding(stmt_.get(), p_tuple, a_tuple);
    return row_range<RowType>(stmt_.get());
  }
  template <typename... Args>
  void execute(Args &&... args) {
    reset_stmt();

    PTuple p_tuple = {};
    tagged_tuple a_tuple{std::forward<Args>(args)...};
    do_binding(stmt_.get(), p_tuple, a_tuple);
    auto r = sqlite3_step(stmt_.get());
    check_sqlite_return(r, SQLITE_DONE);
  }
};

template <fixed_string S, typename T>
decltype(auto) field(T &&t) {
  return skydown::sqlite_experimental::get<
      compile_string<fixed_string<S.size()>(S)>>(std::forward<T>(t));
}

template <fixed_string S, typename T>
auto bind(T &&t) {
  return make_member<compile_string<fixed_string<S.size()>(S)>>(
      std::forward<T>(t));
}

}  // namespace sqlite_experimental

using sqlite_experimental::bind;
using sqlite_experimental::field;
using sqlite_experimental::prepared_statement;

}  // namespace skydown
