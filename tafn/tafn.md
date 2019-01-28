# Types as Function Names

## TLDR:

A single general addition to the C++ core language will be able to solve what are currently viewed as multiple independent problems:

* Universal Function Call Syntax
* Extension Methods
* Deducing This
* Extension points
* Smart references/ proxies
* Forwarding member functions calls to contained objects
* Runtime polymorphism without inheritance


## Motivation

Using names to designate functions is a source of limitations for C++. The limitations fall into three main categories:

1. Backwards compatibility limitations
	* Universal Function Call Syntax
	* Extension Methods
	* Deducing This


2. Incomplement namespacing
	* Extension points

3. Lack of meta-programmability
    * Overload sets
	* Smart references/proxies
	* Making containment easier to use vs. inheritance
	* Runtime polymorphism without inheritance


## Definitions

Let `action tag` be defined as an empty struct:

```
struct foo{}; // foo is an action tag.
struct bar{}; // bar is another action tag.

```

## Proposed calling syntax

Anywhere where a `function name` is used to call a function or member function, `<action tag>` may be used instead. In addition, if `o` is a object, then `o.<action tag>` is equivalent of `<action tag>(o)`

```
// Declaration
void free_function();

free_function();

// Definition of action tag type
struct foo{}; 

<foo>(); // calling an action tag

// Declaration
struct object{
	void member_function();
};

object o;
o.member_function;


// Definition of action type tag.
struct bar{};

// The following 2 calls are equivalent.
o.<bar>();
<bar>(o);


```

## Proposed defintion syntax

Let `object` be an object type, and `foo` be an n `action tag` and `ActionTag` be generic `action tag`:

Then an implementation of an `action tag` can be accomplished in 4 ways:

1. Implementing a specific `action tag` for a specific type.

```

auto <foo,object>(object& o){
	return 43;
}

```

2. Implementing all `action tags` for a single type.

```
template<typename ActionTag>
auto <ActionTag, object>(object& o){
	return 42;
}

```

3. Implementing an `action tag` for all types.


```
template<typename T>
auto <foo>(T& t){
	return 42;
}

```

4. Implementing all `action tags` for all types.

```
template<typename ActionTag, typename T>
auto <ActionTag>(T& t){
	return 42;
}

```

Implementation 1 takes precedence over 2, 2 takes precedence over 3, 3 takes precedence over 4. SFINAE and concepts may be used to constrain the implementation. An implementation may be a friend of a class. Within predence levels 1-4, lookup proceeds in a manner consistent with ADL, with the addition of the namespace of the `action tag` to the list of namespaces searched.

## Rationale for taking optional object type.

This allows us to convert member functions to use `action tags` without having to worry about having a better overload hiding our function. This is illustrated below:

```
struct object{
    int foo(double){
        return 42;
    }
};

int foo(object&, double){
    return 42;
}

template<typename T>
int foo(T&, int){
    return 1;
}



int main(){
    object o;
    o.foo(1); // returns 42
    foo(o,1); // return 1
}


```

Without that extra object type, users would not be able to confidently convert member functions to the `acton tag` format.
## Supporting types

### is_action_tag_invocable

```

// iff <ActionTag>(decltype<Args>()...) is well-formed.
template<typename ActionTag, typename... Args>
struct is_action_tag_invocable :std::true_type{}

// otherwise
template<typename ActionTag, typename... Args>
struct is_action_tag_invocable :std::false_type{}


template<typename ActionTag, typename... Args>
inline constexpr bool is_action_tag_invocable_v = is_action_type_invocable<ActionTag,Args...>::value;


```

### action_tag_overload_set

A functor that forwards all arguments to an 	`action tag`

```
template<typename ActionTag>
struct action_tag_overload_set{
	template<typename... T, typename = std::enable_if_t<is_action_tag_invocable_v<ActionTag,T...>>
	decltype(auto) operator()(T&&... t) const{
		return <ActionTag>(std::forward<T>(t)...);
	}
};

```

# Applications

## Overcoming Backword Compatibility Limitations

### Universal Function Call Syntax

While universal function call syntax would be very useful, there are backward compatiblity issues. Using `action tags` because we do not have to worry about backward compatiblity we can define that `o.<foo>()` and `<foo>(o)` are equivalent. In addition, the extra object type in the definition of the implementation of the `action tag` allows confident conversion of member functions to `action tags`.


### Extension methods

Along the same lines of what we are doing, we can easily add new methods to a type. The nice thing is that since we are using types, we can use namespacing to make sure they don't conflict with future methods.

For example, wouldn't it be nice for `int` to have a `to_string` method?

```

namespace my_methods{

struct to_string{};

auto <to_string,int>(int i) -> std::string{
  return std::to_string(i);
}

}

5.<my_methods::to_string()>();



```

We could even extend this and provide a default to_string implementation.

```

template<typename T>
auto <to_string>(T& t) -> std::string{
	std::stringstream s;
	s << t;
	return s.str();
}

```

### Deducing this

Let's say there object has a data_ member that we would like to expose via a member function.
```
class object{
string data_;

public:
const string& data() const &{return data_;}
string& data() &{return data_;}
string&& data()&& {return data_;}

```

We have the same function body, but have to have different qualifiers. Here is how to do it with this proposal.

```
struct get_data{};
class object{
string data_;

template<typename Self>
friend decltype(auto) <get_data, object>(Self&& self){
  return (std::forward<Self>(self).data_);
}
};
```

## Overcoming incomplete namespacing

### Customization points
Whenever we define a free function that is called through ADL, we are essentially reserving that name across namespaces. An illustration is from https://quuxplusone.github.io/blog/2018/06/17/std-size/ . There was a library that defined `size` in it's own namespace, which compiled fine with C++11. However, C++17 introduced `std::size` which then caused a conflict. With action tags, this would not be an issue. For example, we could put extension points in a hypothetical namespace.

```
namespace std::extension_points{
	struct size{};

    template<typename T>
	auto <size>(T& t){
		return t.size();
	}
}

```

Which could then be called 

```

std::vector v{1,2,3,...};

<std::extension_points::size>(v);

```

in addition, because with `action tags` the namespace of the tag is taken into account for ADL lookup, we do not have to do the `using std::extension_point` dance.

```
// Not needed any more with actino tags
using std::size;
size(v);




```

## Overcoming lack of meta-programmability

### Overload sets

When calling a generic function such as `transform` it is not convenient to pass in an overloaded function. We either have to specify the exact parameters, or wrap in a lambda. With this proposal, any `action tag` overload set can easily be passed to a generic overload using `action_tag_overload_set`. So if you have `action tag` `foo`, you can call 

```
transform(v.begin(),b.end(), action_tag_overload_set<foo>{});
```


### Smart references and proxies

While you can easily define a smart pointer in C++ by overloading `operator->()`, there is no easy way to define a smart reference that will forward the member function calls. With this proposal, you can write smart reference that forwards `action tags`.

```

nameespace dumb_reference { 

struct reset{};

template<typenaem T>
class basic_dumb_reference{
	T* t_;

public:
	template<typename ActionTag, typename Self, typename... T, typename = std::enable_if_t<is_action_tag_invocable_v<ActionTag,Self,T...>>>
	friend decltype(auto)  <ActionTag,basic_dumb_reference<T>>(Self&& self, T&&... t){
		return <ActionTag>(self.t_,std::forward<T>(t)...);
	}

	auto <reset, basic_dumb_reference<T>>(T& t) -> void{
		t_ = &t;
	}
};

int i = 0;
int j = 2;

dumb_reference::basic_dumb_reference<int> ref;
ref.<dumb_reference::reset>(i);
ref.<to_string>(); // "0"
ref.<dumb_reference::reset>(j);
ref.<to_string>(); // "2"

}

```

In addition to forwarding easily, `action tags` also solve the issue of how to differentiate the member functions of the smart reference itself. Because `action tags` are namespaced, it is obvious that the `action tag` `reset` applies to the reference itself and not to what is being stored.

In addition, if we could extend this tecnhique to making for example debugging proxies doing such things as logging all `action tags` and parameters if we so desired.

### Containment

One of the reasons inheritance is used over containment is the convenience of not having to manually forward member function calls. With `action tags` you can automatically forward all non-implemented `action tags` to the contained object.

```

struct object{
	contained_object contained_;

   template<typename ActionTag, typename Self, typename... T, typename = std::enable_if_t<is_action_tag_invocable_v<ActionTag,decltype(declval<Self>().contained_),T...>>>
	friend decltype(auto) <ActionTag,object>(Self&& self, T&&... t){
		return <ActionTag>(self.contained_,std::forward<T>(t)...);
	}
}


```

### Polymorphism without inheritance

You can use template metaprogramming to define a `polymorphic_object` that takes `action tags` signatures.

```

template<typename ReturnType, typename ActionTag, typename... Parameters>
struct signature{};

template<typename... Signatures>
struct polymorphic_object{

};


// Action tags
struct foo{};
struct bar{};


using foo_and_barable = polymorphic_object<signature<void,foo>,signature<void,bar,int>>;

std::vector<foo_and_barable> stuff;

for(auto& v: stuff){
	v.<foo>();
	v.<bar>(1);
}



```

This instead of basing polymporphism on inheritance patterns, it can be based on support for invoking an `action tag` with a set of parameters.


# Implementability, proof of concept

I am not a compiler engineer and have not been able to implement this in a compiler. However, as a proof of concept I simulated this using a library https://github.com/google/cpp-from-the-sky-down/tree/master/tafn.

On the calling side, instead of 

```
o.<foo>();
<foo>(o);

```

It uses:

```
using tafn::_;

o *_<foo>();
*_<foo>(o);

```

On the defintion side:

Instead of 

```
auto <foo, object>(object& o){return 42;}

```

It has:

```
auto  tafn_customization_point(foo, tafn::type<object>,object &) {
			return 42;
}

```

Using that library, I have been able to implement many of the applications of this technique mentioned in this proposal. For the ones I have not yet implemented (mainly polymorphic object), I can see a clear path towards how it could be done.


