# Types as function names

## TLDR:

Addition of a new call syntax `<type>(args)` is proposed (NB: the angle brackets are part of
the actual syntax). After defining the semantics of this construct, examples are provided how
this new call syntax can be used to address the following issues in C++:
* Universal Function Call Syntax
* Extension Methods
* Deducing This
* Extension points
* Smart references/ proxies
* Forwarding member functions calls to contained objects
* Runtime polymorphism without inheritance
* Exception and non-exception based overloads
* Adding monadic bind to pre-existing monad types

## Motivation
Currently we use identifiers to designate free and member functions. However, this is a source
of limitations for C++. We are limited due to the need to maintain backwards compatibility,
incomplete namespacing, and lack of meta-programmability for these names. For each of these
limitations, there are features that we would like to have, but we do not.
1. Backwards compatibility limitations
* Universal Function Call Syntax
* Extension Methods
* Deducing This
2. Incomplete namespacing
* Extension points
3. Lack of meta-programmability
* Overload sets
* Smart references/proxies
* Making containment easier to use vs. inheritance
* Runtime polymorphism without inheritance
* Exception and non-exception based overloads
* Adding monadic bind to pre-existing monad types
* Making a monadic type automatically map/bind

All these issues can be addressed if we allow functions to be designated using types.
## Definitions
Let action tag be defined as struct declaration (See Alternative to action tag as type at the end
of the paper for an alternative).
```
struct foo; // foo is an action tag.
struct bar; // bar is another action tag.
```

### Proposed calling syntax
Anywhere where a function name is used to call a function or member function, `<action tag>`
may be used instead. In addition, `t.<action tag>(args..)` is equivalent of `<action tag>(t,args..)`.
```
// Declaration
void free_function();
free_function();
// Definition of action tag type
struct foo;
<foo>(); // calling an action tag
// Declaration
struct object{
  void member_function();
};
object o;
o.member_function();

// Declaration of action type tag.
struct bar;
// The following 2 calls are equivalent.
o.<bar>();
<bar>(o);

```

Proposed definition syntax
Let object be an object type, and foo be an action tag and ActionTag be a generic action tag:
Then an implementation of an action tag can be accomplished in 4 ways:
1. Implementing a specific action tag for a specific type.
```
auto <foo,object>(object& o){
  return 43;
}
```
2. Implementing all action tags for a single type.
```
template<typename ActionTag>
auto <ActionTag, object>(object& o){
  return 42;
}
```
3. Implementing an action tag for all types.
```
template<typename T>
auto <foo>(T& t){
  return 42;
}
```
4. Implementing all action tags for all types.
template<typename ActionTag, typename T>
 ```
auto <ActionTag>(T& t){
  return 42;
}
```
Implementation 1 takes precedence over 2, 2 takes precedence over 3, 3 takes precedence
over 4. SFINAE and concepts may be used to constrain the implementation. An implementation
may be a friend of a class. Within predence levels 1-4, lookup proceeds in a manner consistent
with ADL, with the addition of the namespace of the action tag to the list of namespaces
searched.

### Rationale for distinction between 1,2 and 3,4.
The question arises if in the proposed definition syntax, we need to differentiate between 1,2
and 3,4 or if partial ordering could be used instead. The issue with partial ordering is that it
treats all parameters as equal, but many times, in practice, we want the first parameter to be
privileged. Consider this example:
```
struct object{
  int foo(double){
    return 42;
  }
};
int foo(object&, double){
  return 42;
}
// Generic implementation
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

Now imagine that someone is writing the same code using action tags
```
// action tag
struct foo;
struct object{
};
// foo implementation for object
auto <foo>(object&, double) -> int{ return 42;}
// foo implementation for any type
template<typename T>
auto <foo>(T&, int){ return 1;}
int main(){
object o;
o.<foo>(1); // returns 1!!!
<foo>(o,1); // returns 1
```

The user is definitely going to be surprised. Looking at the code, it is clear that the 2 parameters
are not of equal importance. The first parameter is more important than the second parameter.
By having the optional object type, we can specify that in the code:
```
// action tag
struct foo;
struct object{
};
// foo implementation for object
auto <foo,object>(object&, double) -> int{ return 42;}
// foo implementation for any type
template<typename T>
auto <foo>(T&, int){ return 1;}
int main(){
  object o;
  o.<foo>(1); // returns 42
  <foo>(o,1); // returns 42
}
```

Without this differentiation, users would not be able to confidently convert member functions to
action tags, without worrying that overload resolution will find a better overload than the action
tag the user explicitly implemented for their type.

### Supporting types
#### is_action_tag_invocable
```
// iff <ActionTag>(decltype<Args>()...) is well-formed.
template<typename ActionTag, typename... Args>
struct is_action_tag_invocable :std::true_type{}
// otherwise
template<typename ActionTag, typename... Args>
struct is_action_tag_invocable :std::false_type{}

template<typename ActionTag, typename... Args>
inline constexpr bool is_action_tag_invocable_v =
is_action_type_invocable<ActionTag,Args...>::value;
```

#### action_tag_overload_set
A class that overloads `operator()` and forwards all arguments to an action tag
```
template<typename ActionTag>
struct action_tag_overload_set{
  template<typename... T, typename =
  std::enable_if_t<is_action_tag_invocable_v<ActionTag,T...>>
  decltype(auto) operator()(T&&... t) const{
    return <ActionTag>(std::forward<T>(t)...);
  }
};
```

## Applications
### Overcoming Backword Compatibility Limitations

#### Universal Function Call Syntax
While universal function call syntax would be very useful, there are backward compatiblity
issues. Using action tags because we do not have to worry about backward compatiblity we can
define that `t.<foo>(args...)` and `<foo>(t,args...)` are equivalent. In addition, the extra object type
in the definition of the implementation of the action tag allows confident conversion of member
functions to action tags.

#### Extension methods
Along the same lines of what we are doing, we can easily add new methods to a type. The nice
thing is that since we are using types, we can use namespacing to make sure they don't conflict
with future methods.
For example, wouldn't it be nice for int to have a `to_string` method?
```
namespace my_methods{
  struct to_string;
  auto <to_string,int>(int i) -> std::string{
    return std::to_string(i);
  }
}
int i = 5;
i.<my_methods::to_string>();
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
This would also be useful for ranges actions and views where instead of of forcing `operator|` into
service with it's precedence quirks, we could actually use something like
```
v.<action::sort>().<action::unique>().<view::filter>([](auto&){/*stuff/*});
```
#### Deducing this
Let's say there object has a data_ member that we would like to expose via a member function.
```
class object{
  string data_;
public:
  const string& data() const &{return data_;}
  string& data() &{return data_;}
  const string& data() const &{return data_;}
  string&& data()&& {return data_;}
  const string&& data()const && {return data_;}
};
```
We have the same function body, copied 4 times to have different qualifiers. Here is how to do it with
this proposal.
```
struct get_data;
class object{
  string data_;
  template<typename Self>
  friend decltype(auto) <get_data, object>(Self&& self){
    return (std::forward<Self>(self).data_);
  }
};
object o;
o.<get_data>();
<get_data>(o); // same
```
### Overcoming incomplete namespacing
#### Customization points
Whenever we define a free function that is called through ADL, we are essentially reserving that
name across namespaces. An illustration is from
https://quuxplusone.github.io/blog/2018/06/17/std-size/ . There was a library that defined size in
it's own namespace, which compiled fine with `C++11`. However, `C++17` introduced `std::size`
which then caused a conflict. With action tags, this would not be an issue. For example, we
could put extension points in a hypothetical namespace.
```
namespace std::extension_points{
  struct size;
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

in addition, because with action tags the namespace of the tag is taken into account for ADL
lookup, we do not have to do the `using std::size/begin/end` dance.
```
// Not needed any more with action tags
using std::size;
size(v);
```
    
### Overcoming lack of meta-programmability

The final limitation of using identifiers for functions is that we cannot use meta-programming
except for the pre-preprocessor token pasting. There is nothing in C++ language outside the
pre-processor that can manipulate a function name. The only thing you can do with a function
name is to call it or get a function pointer (which you may not be able to do if it is overloaded).
The proposed feature would allow meta-programming and open up the following features.
#### Overload sets
When calling a generic function such as `transform` it is not convenient to pass in an overloaded
function. We either have to specify the exact parameters, or wrap in a lambda. With this
proposal, any action tag overload set can easily be passed to a generic overload using
`action_tag_overload_set`. So if you have action tag `foo`, you can call
`transform(v.begin(),b.end(), action_tag_overload_set<foo>{});`
#### Smart references and proxies
While you can easily define a smart pointer in C++ by overloading `operator->()`, there is no easy
way to define a smart reference that will forward the member function calls. With this proposal,
you can write smart reference that forwards action tags.
```
nameespace dumb_reference {
  // Action tag which resets the dumb reference. Applies to the dumb reference itself.
  struct reset;
  template<typename T>
  class basic_dumb_reference{
    T* t_;
    public:
    template<typename ActionTag, typename Self, typename... T,
    typename =
    std::enable_if_t<is_action_tag_invocable_v<ActionTag,Self,T...>>>
    friend decltype(auto) <ActionTag,basic_dumb_reference<T>>(Self&&
    self, T&&... t){
      return <ActionTag>(self.t_,std::forward<T>(t)...);
    }
    friend auto <reset, basic_dumb_reference<T>>(T& t) -> void{
      t_ = &t;
    }
  };
}
int i = 0;
int j = 2;
dumb_reference::basic_dumb_reference<int> ref;
ref.<dumb_reference::reset>(i);
ref.<to_string>(); // "0"
ref.<dumb_reference::reset>(j);
ref.<to_string>(); // "2"
}
```


In addition to forwarding easily, action tags also solve the issue of how to differentiate the
member functions of the smart reference itself. Because action tags are namespaced, it is
obvious that the action tag reset applies to the reference itself and not to what is being stored.
In addition, if we could extend this tecnhique to making for example debugging proxies doing
such things as logging all action tags and parameters if we so desired. In fact, we could even
use this to create a transparent remoting proxy which will convert member function invocations
into a remote procedure call.

#### Containment

One of the reasons inheritance is used over containment is the convenience of not having to
manually forward member function calls. With action tags you can automatically forward all
non-implemented action tags to the contained object.
```
struct object{
  contained_object contained_;
  template<typename ActionTag, typename Self, typename... T,
  typename=std::enable_if_t<is_action_tag_invocable_v<
   ActionTag,decltype(declval<Self>().contained_),T...>>>
  friend decltype(auto) <ActionTag,object>(Self&& self, T&&... t){
    return <ActionTag>(self.contained_,std::forward<T>(t)...);
  }
};
```

#### Polymorphism without inheritance
You can use template metaprogramming to define a polymorphic_object that takes action tags
signatures.
```
template<typename ReturnType, typename ActionTag, typename...
Parameters>
struct signature{};
template<typename... Signatures>
struct polymorphic_object{
  // Implementation unspecified.
};

// Action tags
struct foo;
struct bar;

using foo_and_barable =
polymorphic_object<signature<void,foo>,signature<void,bar,int>>;
std::vector<foo_and_barable> stuff;
for(auto& v: stuff){
  v.<foo>();
  v.<bar>(1);
}
```

Thus instead of basing polymporphism on inheritance patterns, it can be based on support for
invoking an action tag with a set of parameters.

#### Exception and non-exception based overloads

Sometimes a class needs both a throwing and a non-throwing version of a member function.
The solution adopted in `std::filesystem` and the networking ts, is to have 2 overloads of each
member function.
```
class my_class{
  void operation1(int i, std::error_code& ec);
  void operation1(int i){
    std::error_code ec;
    operation1(i,ec);
    if(ec) throw std::system_error(ec);
  }
  // And so on for all supported operations
};
```
There is a lot of boilerplate. Using this proposal, we can automatically generate the throwing
versions from the non-throwing versions.
```
#include <system_error>
struct operation1;
struct operation2;

class my class{
// stuff
public:
  // std::error_code based error handling
  friend auto <operation1,my_class>(my_class& c, int i, std::error_code& ec)->void;

 // std::error_code based error handling
 friend auto <operation2, my_class>(my_class& c, int i,int j, std::error_code& ec) -> void;

};

// Automatically generate the throwing counterparts, when an action
// tag is called without passing in an std::error_code.
template <typename ActionTag, typename... Args, typename =
std::enable_if_t<!std::disjunction_v<std::is_same<std::error_code,std:
:decay_t<Args>>...>>, typename =
std::enable_if_t<is_action_tag_invocable_v<ActionTag, dummy&,
Args..., std::error_code&>>
>
decltype(auto) <ActionTag,my_class>(Args&&... args) {
  std::error_code ec;
  auto call = [&]() mutable {
    return <ActionTag>(std::forward<Args>(args)..., ec);
  };

  using V = decltype(call());
  if constexpr (!std::is_same_v<void, V>) {
    V r = call();
    if (ec) throw std::system_error(ec);
    return r;
  }
  else {
    call();
    if (ec) throw std::system_error(ec);
  }
}

```

#### Adding monadic bind to existing monad types.
Let's say we have this tree node type.
```
// Action tags
struct get_parent;
struct get_child;

class node{
  node* parent;
  std::vector<node*> children;
public:
  friend auto <get_parent, node>(const node& self)->node*{
    return parent;
  }
  friend auto <get_child, node>(const node& self, int i)->node*{
    if(i >= 0 && i < children.size())
      return children[i];
    else
      return nullptr;
  }
};
// We can use like this
node n;
node* parent = n.<get_parent>();
// Get the first child
node* child = n.<get_child>(0);

```

However, if we want to get the grandparent, we have to write code like
```
node* parent = n.<get_parent>();
node* grandparent = parent?parent-><get_parent>():nullptr;
```


However, we observe that the pointer returned is a monad. We can use this to write a monadic
bind.
```
template<typename ActionTag>
struct and_then;

template <typename ActionTag, typename T, typename... Args>
auto <and_then<ActionTag>>(T&& t, Args&&... args) 
  ->decltype(<ActionTag>(*std::forward<T>(t),std::forward<Args>(args)...)){
  if(t){
    return <ActionTag>(*std::forward<T>(t),std::forward<Args>(args)...);
  }
  else{
    return nullptr;
  }
}
```

The we can do stuff like
```
node n;
auto great_grandparent =
n.<get_parent>().<and_then<get_parent>>().<and_then<get_parent>>();
auto grand_child = n.<get_child>(0).<and_then<get_child>>(0);
```

This is much simpler, and less error prone than before. In addition, even if we change
get_parent and get_child to return `std::shared_ptr<node>` instead of `node*` our code will still
work.

#### Making a monadic type automatically map/bind

In fact, we could take this even further, and implement monadic binding and functor mapping at
a type level. Using `std::optional` as an example:
```
template<typename T>
struct wrap_optional{
  using type = std::optional<T>;
};
template<typename T>
struct wrap_optional<std::optional<T>>{
  using type = std::optional<T>;
};

template<typename T>
using wrap_optional_t = typename wrap_optional<T>::type;

template<typename ActionTag, typename T, typename Self, typename...Args>
auto <ActionTag,std::optional<T>>(Self&& self, Args&&... args)
 -> wrap_optional_t<decltype(<ActionTag>(*std::forward<Self>(self),std::forward<Args>(args)...))>{
  if(!self){
    return std::nullopt;
  }
  return <ActionTag>(*std::forward<Self>(self),std::forward<Args>(args)...);
}
```

Then we could do the following
```
// Reads a optional<string> from istream
struct read_string;
auto <read_string>(std::istream&) -> std::optional<std::string>;

// Converts from a string to optional<T>
template<typename T>
struct from_string;
template<typename T>
auto <from_string<T>>(std::string_view s) -> std::optional<T>;

// Squares an integer
struct square;
auto <square>(int i) -> int;

int main(){
  auto opt_squared =
    std::cin.<read_string>()
            .<from_string<int>()
            .<square>();
}
```

## Implementability, proof of concept
I am not a compiler engineer and have not been able to implement this in a compiler. However,
as a proof of concept I simulated this using a library
https://github.com/google/cpp-from-the-sky-down/tree/master/tafn.
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
auto tafn_customization_point(foo, tafn::type<object>,object &) {
  return 42;
}
```
Using that library, I have been able to implement many of the applications of this technique
mentioned in this proposal. For the ones I have not yet implemented (mainly polymorphic
object), I can see a clear path towards how it could be done.

## Possible extensions: Calling existing functions
NB: The above proposal would be useful in and of itself. However, how do we use these
mechanisms to optin to calling existing code. Below is a proposal for doing this.
`C++20` will allow for user defined template parameters. One example from
http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2018/p0732r2.pdf is `std::fixed_string`.
Based on that we could define the following.
```
template<std::fixed_string function_name>
struct function_literal_name{};
```
The calling syntax would be:
```
std::vector<int> v;
v.<function_literal_name<"clear">>();
<function_literal_name<"clear">>(v);
```

The implementation would be compiler defined and have the following semantics.

Let `function_name` be the value of `std::fixed_string`, and `o` be the first parameter.
If `o.function_name(parameters...)` is well formed, then this is called. Else if `function_name(o,
parameters...)` is well-formed then it is called, otherwise ill-formed.
In the above example `vector::clear` is called.
This would allow such advantages as allowing UFCS, and forwarding for existing code that uses
function names for new code that opts in using the action tag syntax via `function_literal_name`.

## Alternative to action tag as struct
Although, a type is used for action tag, the fact that action tags are types is actually incidental to
their use. Action tags are never instantiated. Here is an alternative, using declaration of a new
kind.
### Alternative definition of action tag
```
auto <foo>; // foo is an action tag.

template<int N>
auto <get>; // action tag taking non-type template parameter.

template<typename T>
auto <numeric_cast>; // action tag taking type template parameter

template<<action_tag>>
auto <and_then>; // action tag taking another action tag as a parmeter
```

### Alternative calling syntax
The calling syntax would be unchanged.

### Alternative definition syntax
Here is how the definitions would change:

1. Implementing a specific action tag for a specific type. This would be unchanged
```
auto <foo,object>(object& o){
  return 43;
}
```
2. Implementing all action tags for a single type.
```
template<<ActionTag>>
auto <ActionTag, object>(object& o){
 return 42;
}
```
3. Implementing an action tag for all types. This would be unchanged.
```
template<typename T>
auto <foo>(T& t){
  return 42;
}
```

4. Implementing all action tags for all types.
```
template<<ActionTag>, typename T>
auto <ActionTag>(T& t){
  return 42;
}
```

### Alternative Example: and_then
```
template <<ActionTag>, typename T, typename... Args>
auto <and_then<ActionTag>>(T&& t, Args&&... args)
 -> decltype(<ActionTag>(*std::forward<T>(t),std::forward<Args>(args)...)){
  if(t){
    return <ActionTag>(*std::forward<T>(t),std::forward<Args>(args)...);
  }
  else{
    return nullptr;
  }
}
```

### Thoughts on alternative syntax
While the alternative definition is interesting, I see the following downsides:
* We would need to introduce a new kind of declaration. We would also have to make sure
that all the template type machinery also works with this
* We have to special case template << action_tag>> for the lexer to not find left shift
(similar to what was done for double closing angle brackets in C++11
* Other than saving some characters, no specific advantages of this approach are obvious
to me.

Given the above, I believe that sticking with actions tags as types makes the most sense.

## Acknowledgements

Thanks to Arthur O'Dwyer, James Dennet, and Richard Smith for providing valuable feedback.
