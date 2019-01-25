# Types as Function Names

## Introduction

We propose a feature that makes C++ more general and extensible and can help with the following features:
* Extension Methods
* Customization Points
* Smart References
* Forwarding member function calls to another class in lieu of inheritance
* Type Erasure
* Function Decorator
* Deducing this to reduce const / reference overloading boilerplate

The feature is to allow types to be called as functions.

## Proposed Syntax

The goal is to use function type tags instead of functions.

### Calling a function type tag.

```
struct foo{};

object o;

o.<foo>();

// same as above
<foo>(o);

```

### Implementing function tag types

This can be done in 4 ways.

1. Implementing a function type tag for a single function tag and type

```

auto <foo,object>(object& o){
	return 43;
}

```

2. Implementing a function tag type for a all functions tags for a give type.

```
template<typename F>
auto <F, object>(object& o){
	return 42;
}

```

3. Implementing a function tag type for all types.


```
template<typename T>
auto <foo>(T& t){
	return 42;
}

```

4. Implementing a function tag type for all types and all functions

```
template<typename F, typename T>
auto <F>(T& t){
	return 42;
}

```

Implementation 1 takes precedence over 2, and so on.

## is_action_invocable

```
template<typename F, typename... Args>
inline constexpr bool is_action_invocable = // true if <F>(decltype<Args>()...) is well-formed.
```




## Simple Example

You have `struct say_hello{};` and `class my_class{string data_;};` 

Let's call `say_hello` on a object of `my_class`.

```
my_class c;
c.<say_hello>("john");
```

To implement, we just provide the following overload in the namespace of either `my_class` for `say_hello`;

```
void <say_hello, my_class>(const my_class& c, std::string_view name){
  std::cout << "hello " << name << "\n";
```

## Deducing this

Going back to `my_class` let's add an access  member function for `data_`. This how we would currently do it.

```
class my_class{
string data_;

public:
const string& data() const &{return data_;}
string& data() &{return data_;}
string&& data()&& {return data_;}

```

We have the same function body, but have to have different qualifiers. Here is how to do it with this proposal.

```
struct get_data{};
class my_class{
string data_;

template<typename Self>
friend decltype(auto) <get_data, my_class>(Self&& self){
  return (std::forward<Self>(self).data_);
}
};
```

You write the overload once, and it just works.

## Smart references

Let's make a dumb smart reference.

```
 template<typename T>
	struct smart_reference {
		T& r_;
	};

	template<typename F, typename T,typename Self, typename... Args, typename = std::enable_if_t<is_action_invocable<F,Args...>>>
	decltype(auto) <F,smart_reference<T>>(Self&& self, Args&&... args) {
		return std::forward<Self>(self).r_.<F>(std::forward<Args>(args)...);
	}


```


Now we can do the following :
```
smart_reference<my_class> ref{ c };
std::string& str = ref.<get_data>();

```

We have implemented a smart reference forwarding in literally 3 lines of code.


### Extension methods

Along the same lines of what we are doing, we can easily add new methods to a type. The nice thing is that since we are using types, we can use namespacing to make sure they don't conflict with future methods.

For example, wouldn't it be nice for `int` to have a `to_string` method?

```

namespace my_methods{

struct to_string{};

std::string <to_string,int>(int i){
  return std::to_string(i);
}

}

5.<my_methods::to_string()>();



```

Let's add an output method that takes an ostream.

```
struct output {};
template<typename T,
	typename =
	std::void_t<decltype(std::declval<std::ostream&>() << std::forward<T>(std::declval<T>()))
	>>
	void <output>(const T& t, std::ostream& os, std::string_view delimit = "") {
	os << t << delimit;
}

```

Then, if we have a variable `x`, instead of doing `std::cout << x << "\n"; ` we can write `x.<output>(std::cout,"\n");`

### Extensions Methods for Collections

With extension methods, we can do the following to read lines from stdin, sort, unique, and output.

```
std::cin.<get_all_lines>().<sort>().<unique>().<call_for_each<output>>(std::cout, "\n");
```

We previously defined output, here are the others.

```

struct get_all_lines {};

std::vector<std::string> <get_all_lines>(std::istream& is) {
	std::vector<std::string> result;
	std::string line;
	while (std::getline(is, line)) {
		result.push_back(line);
	}
	return result;
}

struct sort {};

template <typename T,typename = std::void_t<decltype(std::begin(std::declval<T>()))>>
decltype(auto) std_customization_point(sort, all_types, T&& t) {
    std::sort(t.begin(), t.end());
    return std::forward<T>(t);
}


struct unique {};

template <typename T, typename = std::void_t<decltype(std::begin(std::declval<T>()))>>
decltype(auto) std_customization_point(unique, all_types, T&& t) {
    auto iter = std::unique(t.begin(), t.end());
    t.erase(iter, t.end());
    return std::forward<T>(t);
}

template<typename F>

struct call_for_each {};
template<typename F, typename C, typename... Args, typename =
	std::enable_if_t<
	is_valid<F, decltype(*std::forward<C>(std::declval<C>()).begin()), Args...>>>
	void std_customization_point(call_for_each<F>, all_types, C&& c, Args&&... args) {
	for (auto&& v : std::forward<C>(c)) {
		call_customization_point<F>(std::forward<decltype(v)>(v), std::forward<Args>(args)...);
	}
}


```


### Customization points

One of the issues with C++ is picking customization points. For example, the addition of `size` as a customization point in `C++17`, resulted in compilation errors in code that worked with `C++14`.
[https://quuxplusone.github.io/blog/2018/11/04/std-hash-value/]

Using type names as functions, would make this problem go away.  Since, these would by defined unambiguously by types.

For example, if we had to add a new customization point `baz` it could be called `std::customization_point::baz` and be called:

`foo.<std::customization_point::baz>();`

Customization points could be added to the standard without any fear that they would conflict with existing code.

### Decorators

TODO

