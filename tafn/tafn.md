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

Assume you have the following template structs;

* `template<typename T> struct type{};`
* `template<typename F> struct all_functions{};`
* `struct all_types{};`

Assume you have the following forms of overloads for `std_customization_point`

1. `template<typename F, typename T, typename... Args> decltype(auto) std_customization_point(F, type<T>, T&& t, Args&&... args);` - implements a specific function F for a single type T.
2. `template<typename F, typename T, typename... Args> decltype(auto) std_customization_point(all_functions<F>, type<T>, T&& t, Args&&... args);` - implements all functions for a single type T.
3. `template<typename F, typename T, typename... Args> decltype(auto) std_customization_point(F, all_types, T&& t, Args&&... args);` - implements a specific function F for all types. 
4. `template<typename F, typename T, typename... Args> decltype(auto) std_customization_point(all_functions<F>, all_types, T&& t, Args&&... args);` - implements all functions for all types.

Assume you have the following callable:

`template<typename F, typename T, typename... Args> decltype(auto) call_customization_point(T&& t, Args&&... args);` which checks if the overload form of `std_customization_point` exists and if it exists calls it. The overload form 1 is higher priority than 2 which is higher than 3 which is higher than 4.

Then if you have Type `F` and Object `o` of Type `T` then `o.<F>(args);` is the same as `call_customization_point<F>(o,args);`

Finally, let us add a template variable which will let us know if we can call `call_customization_point`.
```
template<typename F, typename... Args>
inline constexpr bool is_valid = // True iff there is an std_customization_point_overload such that call_customization_point<F>(declval<Args>()...) is well-formed
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
void std_customization_point(say_hello, type<my_class>, const my_class& c, std::string_view name){
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

template<typename T>
friend decltype(auto) std_customization_point(get_data, type<my_class>, T&& t){
  return (std::forward<T>(t).data_);
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

	template<typename F, typename T,typename Self, typename... Args, typename = std::enable_if_t<is_valid<F,,Args...>>>
	decltype(auto) std_customization_point(all_functions<F>, type<smart_reference<T>>, Self&& self, Args&&... args) {
		return call_customization_point<F>(std::forward<Self>(self).r_, std::forward<Args>(args)...);
	}


```


Now we can do the following :
```
smart_reference<my_class> ref{ c };
std::string& str = ref.<get_data>();

```

We have implemented a smart reference forwarding in literally 3 lines of code.

We can even do it for  `std::shared_ptr`

```


	template<typename F, typename T,typename Self, typename... Args, typename = std::enable_if_t<is_valid<F,T&,Args...>>>
	decltype(auto) std_customization_point(all_functions<F>, type<shared_ptr<T>>, Self&& self, Args&&... args) {
		return call_customization_point<F>(*std::forward<Self>(self), std::forward<Args>(args)...);
	}


```

For this, we would have to be in namespace std (so this overload would have be be provided by the standard library).

Once we have this we could do:

```

auto ptr = std::make_shared<my_class>();

ptr.<get_data>();


``` 


### Extension methods

Along the same lines of what we are doing, we can easily add new methods to a type. The nice thing is that since we are using types, we can use namespacing to make sure they don't conflict with future methods.

For example, wouldn't it be nice for `int` to have a `to_string` method?

```

namespace my_methods{

struct to_string{};

std::string std_customization_point(to_string, type<int>,int i){
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
	void std_customization_point(output, all_types, const T& t, std::ostream& os, std::string_view delimit = "") {
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

std::vector<std::string> std_customization_point(get_all_lines, all_types, std::istream& is) {
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

### Decorators

TBD

