// tagged_sqlite.h : Include file for standard system include files,
// or project specific include files.

#pragma once
#include <assert.h>
#include <sqlite3.h>

#include <array>
#include <cstdint>
#include <iostream>
#include <optional>
#include <string>
#include <type_traits>
#include <vector>

#include "..//simple_type_name/simple_type_name.h"
#include "..//tagged_tuple/tagged_tuple.h"

namespace skydown {

template <typename T>
void check_sqlite_return(T r, T good = SQLITE_OK) {
  assert(r == good);
  if (r != good) {
    std::cerr << "SQLITE ERROR " << r;
    throw std::runtime_error("sqlite error");
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
  check_sqlite_return(count, tuple_size(row));
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

  void next() { last_result = sqlite3_step(stmt); }

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

inline bool bind_impl(sqlite3_stmt *stmt, int index, const std::string_view v) {
  auto r = sqlite3_bind_text(stmt, index, v.data(), v.size(), SQLITE_TRANSIENT);
  return r == SQLITE_OK;
}

// Beginning of experimental option of parsing columns and parameters from the
// sql statement string_view.
// <:column:type> <:p:parameter:Type>
// select <:id>, long_name as <:ls:s> from table where id = <:p:parm:i>
#include <string_view>
#include <utility>

namespace sqlite_experimental {

template <char... c>
struct compile_string {};

template <auto &sv, std::size_t... I>
auto to_compile_string_helper(std::index_sequence<I...>) {
  constexpr auto svp = sv;
  return compile_string<svp[I]...>{};
}

template <auto &sv>
constexpr auto to_compile_string() {
  constexpr auto svp = sv;
  return to_compile_string_helper<sv>(std::make_index_sequence<svp.size()>());
}

template <auto &sv>
using compile_string_sv = decltype(to_compile_string<sv>());

template <bool make_optional, typename Tag, typename T>
auto maybe_make_optional(member<Tag, T> m) {
  if constexpr (make_optional) {
    return member<Tag, std::optional<T>>{};
  } else {
    return m;
  }
}

template <typename T>
struct string_to_type;

template <>
struct string_to_type<compile_string<'i', 'n', 't'>> {
  using type = std::int64_t;
};

template <>
struct string_to_type<compile_string<'s', 't', 'r', 'i', 'n', 'g'>> {
  using type = std::string_view;
};

template <>
struct string_to_type<compile_string<'d', 'o', 'u', 'b', 'l', 'e'>> {
  using type = double;
};

template <typename T>
using string_to_type_t = typename string_to_type<T>::type;

constexpr std::string_view start_group = "{{";
constexpr std::string_view end_group = "}}";

template <const std::string_view &parm,bool is_parms>
constexpr auto get_type_spec_count(std::string_view start_group,
                                   std::string_view end_group) {
  constexpr auto sv = parm;
  std::size_t count = 0;
  for (std::size_t i = sv.find(start_group); i != std::string_view::npos;) {
      if (is_parms == (sv[i+start_group.size()] == '?')) {
      ++count;
    }
    i = sv.find(start_group, i + 1);
  }

  return count;
}

struct type_spec {
  std::string_view name;
  std::string_view type;
  bool optional;
  constexpr type_spec(std::string_view n, std::string_view t, bool o)
      : name(n), type(t), optional(o) {}
  constexpr type_spec() : name(), type(), optional(false) {}
};

constexpr type_spec parse_type_spec(std::string_view sv) {
  auto colon = sv.find(":");
  auto name = sv.substr(0, colon);

  bool optional = sv[sv.size() - 1] == '?' ? true : false;
  auto last_colon = sv.find_last_of(':');
  auto size = sv.size();
  auto new_size = size - (optional ? 2 : 1);
  std::string_view type_str = sv.substr(last_colon + 1, new_size - last_colon);
  return {name, type_str, optional};
}

template <const std::string_view &parm,bool is_parms>
constexpr auto parse_type_specs() {
  constexpr auto sv = parm;
  constexpr auto size = get_type_spec_count<parm,is_parms>(start_group, end_group);
  std::array<type_spec, size> ar = {};
  std::size_t count = 0;
  for (std::size_t i = sv.find(start_group); i != std::string_view::npos;) {
    auto end = sv.find(end_group, i);
    auto start = i + start_group.size();
    if (is_parms == (sv[start] == '?')) {
      if (is_parms)++start;
      ar[count] = parse_type_spec(sv.substr(start, end - start));
      ++count;
    }
    i = sv.find(start_group, i + 1);
  }

  return ar;
}

template <const auto &parm>
constexpr auto make_member_ts() {
  constexpr static auto ts = parm;
  constexpr static auto name = ts.name;
  constexpr static auto type = ts.type;
  using name_t = compile_string_sv<name>;
  using type_str_t = compile_string_sv<type>;
  return maybe_make_optional<ts.optional>(
      make_member<name_t>(string_to_type_t<type_str_t>{}));
}

template <const auto &parm, typename S>
struct make_members_struct;

template <const auto &parm, std::size_t... I>
struct make_members_struct<parm, std::index_sequence<I...>> {
  constexpr static auto ar = parm;
  template <std::size_t i>
  constexpr static auto helper() {
    constexpr auto lar = ar;
    constexpr static auto a = lar[i];
    return make_member_ts<a>();
  }

  constexpr static auto make_members_ts() {
    return tagged_tuple{helper<I>()...};
  }
};

template <const std::string_view &parm>
constexpr auto make_members() {
  constexpr auto sv = parm;
  constexpr static auto ar = parse_type_specs<parm,false>();
  constexpr auto size = ar.size();
  using sequence = std::make_index_sequence<size>;
  using mms = make_members_struct<ar, sequence>;
  return mms::make_members_ts();
}

template <const std::string_view &parm>
constexpr auto make_parameters() {
  constexpr auto sv = parm;
  constexpr static auto ar =
      parse_type_specs<parm,true>();
  constexpr auto size = ar.size();
  using sequence = std::make_index_sequence<size>;
  using mms = make_members_struct<ar, sequence>;
  return mms::make_members_ts();
}

// todo make constexpr
inline std::string get_sql_string(std::string_view sv,
                                  std::string_view start_group,
                                  std::string_view end_group) {
  std::string ret;
  std::size_t prev_i = 0;
  for (std::size_t i = sv.find(start_group); i != std::string_view::npos;) {
    ret += std::string(sv.substr(prev_i, i - prev_i));
    auto end = sv.find(end_group, i);

    ret += " ";
    auto ts_str =
        sv.substr(i + start_group.size(), end - (i + start_group.size()) - 1);
    auto ts = parse_type_spec(ts_str);
    if (ts.name.front() != '?') {
        ret += std::string(ts.name);
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
  skydown::for_each(p_tuple, [&](auto &m) mutable {
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

template <typename RowType, typename PTuple>
struct prepared_statement {
  unique_stmt stmt;
  void reset_stmt() {
    auto r = sqlite3_reset(stmt.get());
    check_sqlite_return(r);
    r = sqlite3_clear_bindings(stmt.get());
    check_sqlite_return(r);
  }
  template <typename... Args>
  row_range<RowType> execute_rows(Args &&... args) {
    reset_stmt();

    PTuple p_tuple = {};
    tagged_tuple a_tuple{std::forward<Args>(args)...};
    do_binding(stmt.get(), p_tuple, a_tuple);
    return row_range<RowType>(stmt.get());
  }
  template <typename... Args>
  bool execute(Args &&... args) {
    reset_stmt();

    PTuple p_tuple = {};
    tagged_tuple a_tuple{std::forward<Args>(args)...};
    do_binding(stmt.get(), p_tuple, a_tuple);
    return sqlite3_step(stmt.get()) == SQLITE_DONE;
  }
};

template <const std::string_view &parm>
auto prepare_query_string(sqlite3 *sqldb) {
  using row_type = decltype(make_members<parm>());

  auto sv = parm;
  sqlite3_stmt *stmt;
  auto query_string = get_sql_string(sv, start_group, end_group);
  auto rc = sqlite3_prepare_v2(sqldb, query_string.c_str(), query_string.size(),
                               &stmt, 0);
  check_sqlite_return(rc);

  using p_tuple_type = decltype(make_parameters<parm>());
  return prepared_statement<row_type, p_tuple_type>{unique_stmt(stmt)};
}

template <const std::string_view &parm, typename... Args>
auto execute_query_string(sqlite3 *sqldb, Args... args) {
  auto row = make_members<parm>();
  tagged_tuple a_tuple{args...};
  auto p_tuple = make_parameters<parm>();

  auto sv = parm;
  sqlite3_stmt *stmt;
  auto query_string1 = get_sql_string(sv, "<:", ">");
  auto query_string = get_sql_string_parms(query_string1, "{:", "}");
  auto rc = sqlite3_prepare_v2(sqldb, query_string.c_str(), query_string.size(),
                               &stmt, 0);
  check_sqlite_return(rc);
  do_binding(stmt, p_tuple, a_tuple);

  return row_range<std::decay_t<decltype(row)>>(stmt);
}

template <typename Tag1, typename Tag2>
constexpr auto concatenate_tags() {
  constexpr auto type_str1 = short_type_name<Tag1>;
  constexpr auto type_str2 = short_type_name<Tag2>;
  std::array<char, type_str1.size() + type_str2.size() + 1> ar = {};
  std::size_t i = 0;
  for (auto c : type_str1) {
    ar[i] = c;
    ++i;
  }
  ar[i] = '.';
  ++i;
  for (auto c : type_str2) {
    ar[i] = c;
    ++i;
  }
  return ar;
}

template <typename Tag1, typename Tag2 = void, typename T>
decltype(auto) field(T &&t) {
  if constexpr (std::is_same_v<Tag2, void>) {
    static constexpr auto type_str = short_type_name<Tag1>;
    return skydown::get<compile_string_sv<type_str>>(std::forward<T>(t));
  } else {
    static constexpr auto ar = concatenate_tags<Tag1, Tag2>();
    using tag = compile_string_sv<ar>;
    return skydown::get<tag>(std::forward<T>(t));
  }
}

template <typename Tag, typename T>
auto bind(T &&t) {
  static constexpr auto type_str = short_type_name<Tag>;
  return skydown::make_member<compile_string_sv<type_str>>(std::forward<T>(t));
}

}  // namespace sqlite_experimental

}  // namespace skydown
