#include <iostream>
#include <string>
#include <assert.h>


#include "tagged_tuple.h"
#include "soa_vector.h"

using ftsd::get;
using ftsd::tagged_tuple;
using namespace ftsd::literals;
using ftsd::auto_;
using ftsd::member;
using ftsd::tag;

using test_arguments =
    tagged_tuple<member<"a", int, [] {}>,  // Required argument.
                 member<"b", auto_, [](auto& t) { return get<"a">(t) + 2; }>,
                 member<"c", auto_, [](auto& t) { return get<"b">(t) + 2; }>,
                 member<"d", auto_, []() { return 5; }>>;

void test(test_arguments args) {
  std::cout << get<"a">(args) << "\n";
  std::cout << get<"b">(args) << "\n";
  std::cout << get<"c">(args) << "\n";
  std::cout << get<"d">(args) << "\n";
}

template <typename T>
void print(const T& t) {
  auto f = [](auto&&... m) {
    auto print = [](auto& m) {
      std::cout << m.key() << ": " << m.value() << "\n";
    };
    (print(m), ...);
  };
  std::cout << "{\n";
  t.apply(f);
  std::cout << "}\n";
}

template <typename TaggedTuple>
auto make_ref(TaggedTuple& t) {
    return ftsd::tagged_tuple_ref_t<TaggedTuple>(t);
}

using ftsd::soa_vector;
void soa_vector_example(){
    using Person = tagged_tuple<
        member<"name",std::string>,
        member<"address",std::string>,
        member<"id", std::int64_t>,
        member<"score", double>>;

    soa_vector<Person> v;

    v.push_back(Person{tag<"name"> = "John", tag<"address"> = "somewhere", tag<"id"> = 1, tag<"score"> = 10.5});
    v.push_back(Person{tag<"name"> = "Jane", tag<"address"> = "there", tag<"id"> = 2, tag<"score"> = 12.5});

    assert(get<"name">(v[1]) == "Jane");

    auto scores = get<"score">(v);
    assert(*std::max_element(scores.begin(),scores.end()) == 12.5);


}

int main() {
    soa_vector_example();
  tagged_tuple<member<"hello", auto_, [] { return 5; }>,
               member<"world", std::string,
                      [](auto& self) { return get<"hello">(self); }>,
               member<"test", auto_,
                      [](auto& t) {
                        return 2 * get<"hello">(t) + get<"world">(t).size();
                      }>,
               member<"last", int>>
      ts{tag<"world"> = "Universe", tag<"hello"> = 1};

  auto ref_ts = make_ref(ts);
 print(ref_ts);


 

  using T = decltype(ts);
  T t2{tag<"world"> = "JRB"};


  ftsd::soa_vector<T> v;

  v.push_back(t2);

  print(v[0]);

  get<"world">(v[0]) = "Changed";

  print(v[0]);

  auto v0 = get<"world">(v);
  v0.front() = "Changed again";
  

  std::cout << get<"world">(v).front();



  return 0;

  std::cout << get<"hello">(ts) << "\n";
  std::cout << get<"world">(ts) << "\n";
  std::cout << get<"test">(ts) << "\n";
  std::cout << get<"hello">(t2) << "\n";
  std::cout << get<"world">(t2) << "\n";
  std::cout << get<"test">(t2) << "\n";

  test({tag<"c"> = 1, tag<"a"> = 5});
  test({tag<"a"> = 1});
  test({"a"_tag = 1});

  tagged_tuple ctad{"a"_tag = 15, "b"_tag = std::string("Hello ctad")};

  std::cout << ts["world"_tag] << "\n";
  std::cout << ts[tag<"world">] << "\n";
  std::cout << ctad["a"_tag] << "\n";
}