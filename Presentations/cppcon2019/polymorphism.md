class: center, middle

# Polymorphism != Virtual
## John R. Bandela, MD

---
# What is polymorphism
--
* "Many Forms"
---
![technically correct](technically_correct.png)
---
# What is polymorphism?
--
##  Providing a single interface to entities of different types
--
###  - Bjarne Stroustrup
---
# OOP did not invent polymorphism
--
## A + B
--
* Integers
--
* Reals
--
* Complex numbers
--
* Vectors
--
* Matrices
---
# Types of polymorphism
--
* Static polymorphism
--
* Dynamic polymorphism


---
# Static polymorphism
--

## Overloading

---
# Dynamic polymorphism
--

## Virtual Functions
---
# What about variant?
--
## Variant
* Types - Closed set
* Operations - Open set


--
## Polymorphism in this talk
* Types - Open set
* Operations - Closed set

---
# Motivating example
--
## Shape
--
* Draw
--
* Get Bounding Box
--
* Translate
--
* Rotate
---
# Overloading
--
* Determines at compile time which function to call based on parameters
--
* Many, many rules, but in general it just works
--
* No runtime overhead
---
# Overloading example
```cpp
class circle{
public: 
  void draw() const;
  box get_bounding_box() const;
  void translate(double x, double y);
  void rotate(double degrees);

};

```
---
# Overloading example
```cpp

class box{
public:
  point upper_left()const;
  void set_upper_left(point);
  point lower_right()const;
  void set_lower_right(point);
  bool overlaps(const box&)const;
  
};
```
---

# Overloading example
```cpp
// Draw only if the shape will be visible.
template<typename Shape>
void smart_draw(const Shape& s, const box& viewport){
  auto bounding_box = s.get_bounding_box();
  if(viewport.overlaps(bounding_box)) s.draw();
}


```

---
# Overloading example
```cpp
// Move and rotate
template<typename Shape>
void spin(const Shape& s){
  s.translate(4,2);
  s.rotate(45);
}


```
---
# How do we use this
```cpp
cirlce c{...};
spin(c);
smart_draw(c,viewport);
```
---
# What about box

```cpp
circle c;
smart_draw(c, viewport);
```
--
#  &#x2705;
---
# What about box

```cpp
box b;
smart_draw(b,viewport);
```

--
# &#x274c; 

---
# What to do

Write a facade?
--

# &#x274c;
---
# Use overloaded customization points
--
```cpp
template<typename Shape>
void smart_draw(const Shape& s, const box& viewport){
  auto bounding_box = s.get_bounding_box();
  if(viewport.overlaps(bounding_box)) s.draw();
}


```
---
# Use overloaded customization points
```cpp
template<typename Shape>
void smart_draw(const Shape& s, const box& viewport){
  auto bounding_box = `s.get_bounding_box()`;
  if(viewport.overlaps(bounding_box)) `s.draw()`;
}


```
---
# Use overloaded customization points
```cpp
template<typename Shape>
void smart_draw(const Shape& s, const box& viewport){
  auto bounding_box = `get_bounding_box(s)`;
  if(viewport.overlaps(bounding_box)) `draw(s)`;
}

```
--
Provide default implementation
```cpp
template<typename Shape>
void draw(const Shape& s){
 s.draw();
}
```
--
Provide box implementation
```cpp

void draw(const box& b){
// Draw box.
}

```
---

# Smart Draw
```cpp
template<typename Shape>
void smart_draw(const Shape& s, const box& viewport){
  auto bounding_box = `get_bounding_box(s)`;
  if(viewport.overlaps(bounding_box)) `draw(x)`;
}


```

---
# Spin
```cpp
// Move and rotate
template<typename Shape>
void spin(const Shape& s){
  `translate(s,4,2);`
  `rotate(s,45);`
}


```
---
# Use
```cpp
cirlce c{...};
spin(c);
smart_draw(c,viewport);
```
--
```cpp
box b{...};
spin(b);
smart_draw(b,viewport);
```
---
# Value semantics

```cpp
circle c1;
```
--
```
circle c2 = c1;
```
--
```
spin(c1);
```
--
```
smart_draw(c1);
smart_draw(c2);

```
---
# Low Coupling
--
```cpp
template<typename Shape>
void smart_draw(const Shape& s, const box& viewport){
  auto bounding_box = `get_bounding_box(s)`;
  if(viewport.overlaps(bounding_box)) `draw(x)`;
}
```
--
## What operations does this depend on?
--
* Get Bounding Box
* Draw

---
# PPP
--
## Purchasing  Power Parity?
--
# &#x274c; 

---
# Parent's Polymorphic Principles (PPP)
--

* The requirement of a polymorphic type, by definition, comes from its use
--

* There are no polymorphic types, only a polymorphic use of similar types

--

From "Inheritance is the base class of evil" by Sean Parent

---
# PPP
```cpp
template<typename Shape>
void smart_draw(const Shape& s, const box& viewport){
  auto bounding_box = `get_bounding_box(s)`;
  if(viewport.overlaps(bounding_box)) `draw(x)`;
}
```
--

#  &#x2705;
---
# What has static polymorphism ever done for us?
--
*  &#x2705; Low boilerplate
--
*  &#x2705; Easy adaptation of existing class
--
*  &#x2705; Value semantics
--
*  &#x2705; Low coupling
--
*  &#x2705; PPP
--
*  &#x2705; Performance
--
 - Obviously, performance. I mean, performance goes without saying.

---
# Why don't we just stop now?

--

* &#x274c; Requires everything to be a template and thus live in headers
```cpp
template<typename Shape>
void smart_draw(const Shape& s, const box& viewport){
```
--

* &#x274c; Can't store in runtime containers
```cpp
vector<???> shapes;
shapes.push_back(circle{});
shapes.push_back(box{});
```

---
# Dynamic polymorphism - virtual
--
## Interface
--
```cpp
struct shape{
```
--
```cpp
  virtual void draw() const = 0;
```
--
```cpp
  virtual box get_bounding_box() const = 0;
```
--
```cpp
  virtual void translate(double x, double y) = 0;
```
--
```cpp
  virtual void rotate(double degrees) = 0;
};
```
---
# Don't forget the virtual destructor!
```cpp
struct shape{
  virtual void draw() const = 0;
  virtual box get_bounding_box() const = 0;
  virtual void translate(double x, double y) = 0;
  virtual void rotate(double degrees) = 0;
  `virtual ~shape(){}`
};
```
---
# Circle
```cpp
class circle:public shape{
public: 
  void draw() const override;
  box get_bounding_box() const override;
  void translate(double x, double y) override;
  void rotate(double degrees) override;
};

```
---
# Smart Draw
```cpp
void smart_draw(const shape& s, const box& viewport){
  auto bounding_box = s.get_bounding_box();
  if(viewport.overlaps(bounding_box)) s.draw();
}
```
--
# &#x2705; No longer a template
---
# Smart Draw
```cpp
void smart_draw(const `shape`& s, const box& viewport){
  auto bounding_box = s.`get_bounding_box()`;
  if(viewport.overlaps(bounding_box)) s.`draw()`;
}
```
--
## &#x274c; Increased coupling
---
# Splitting Shape
```cpp
struct shape{
  virtual void draw() const = 0;
  virtual box get_bounding_box() const = 0;
  virtual void translate(double x, double y) = 0;
  virtual void rotate(double degrees) = 0;
  virtual ~shape(){}
};
```
---
# Splitting Shape
```cpp
struct shape{
*  virtual void draw() const = 0;
*  virtual box get_bounding_box() const = 0;
  virtual void translate(double x, double y) = 0;
  virtual void rotate(double degrees) = 0;
  virtual ~shape(){}
};
```
---
# Splitting Shape
```cpp
struct drawing_interface{
  virtual void draw() const = 0;
  virtual box get_bounding_box() const = 0;
  virtual ~drawing_interface(){}
};
```
--
```cpp
struct transforming_interface{
  virtual void translate(double x, double y) = 0;
  virtual void rotate(double degrees) = 0;
  virtual ~transforming_interface(){}
};
```
--
```cpp
struct shape: drawing_interface, transforming_interface{};
```
---
# Smart Draw
```cpp
void smart_draw(const `drawing_interface`& s, const box& viewport){
  auto bounding_box = s.get_bounding_box();
  if(viewport.overlaps(bounding_box)) s.draw();
}
```
---
# Spin
```cpp
void spin(const `transforming_interface`& s){
  s.translate(4,2);
  s.rotate(45);
}
```
---
# Use

```cpp
std::vector<std::unique_ptr<shape>> shapes;

for(auto& s:shapes){
  spin(*s);
  smart_draw(*s,viewport);
  
}


```
---
# Use

```cpp
* std::vector<std::unique_ptr<shape>> shapes;

for(auto& s:shapes){
  spin(*s);
  smart_draw(*s,viewport);
  
}

```
--
# &#x2705; Able to be stored in runtime containers
---
# Did we do it?
--
## &#x2705; Able to be used in non-template functions.
--
## &#x2705; Able to be stored in runtime containers
---
![thanos](thanos.jpg)

---
# Costs
--
*  &#x274c; Low boilerplate - Need inheritance hierarchy
--
*  &#x274c; Easy adaptation of existing class - Have to inherit from base
--
*  &#x274c; Value semantics - Pointer semantics
--
*  &#x2705; &#x274c;  Low coupling - Only if we are careful/lucky
--
*  &#x274c; PPP - Polymorphic types have to inherit from base
--
*  &#x2705; &#x274c; Performance - Inherent overhead in runtime dispatch

---
# What would we like
--
*  &#x2705; Low boilerplate
--
*  &#x2705; Easy adaptation of existing class
--
*  &#x2705; Value semantics
--
*  &#x2705; Low coupling
--
*  &#x2705; PPP
--
*  &#x2705; &#x274c; Performance - Inherent overhead in runtime dispatch
--
* &#x2705; Able to be used in non-template functions.
--
* &#x2705; Able to be stored in runtime containers

---
# Constraints
--
* No MACROS
--
* Standard C++17

---

# There is already a solution in the C++11 stdlib
---
# History lesson
--
## 1998
--
```cpp
struct CommandInterface{virtual void execute() = 0;}
```
--
```cpp
std::vector<CommandInterface*> commands;
```
--
```cpp
for(int i = 0; i < commands.size(); ++i)commands[i]->execute();
```
--
## 2011
--
```cpp
std::vector<`std::function<void()>`> commands;
```
--
```cpp
for(auto& command:commands)command();
```
---
# std::function
--
*  &#x2705; Low boilerplate
--
*  &#x2705; Easy adaptation of existing class
--
*  &#x2705; Value semantics
--
*  &#x2705; Low coupling
--
*  &#x2705; PPP
--
*  &#x2705; &#x274c; Performance - Inherent overhead in runtime dispatch
--
* &#x2705; Able to be used in non-template functions.
--
* &#x2705; Able to be stored in runtime containers

---
# std::function
```cpp
std::function<`void()`> f;
```
--
## Function type
* Has the information on the parameters and return type of the function
--
* We, can examine the parameters and return type, using partial template specialization.
---
# function_ref
--
* For passing to functions, sometimes you just need a reference type, instead of an owning type
--
* Ongoing work on standardizing, but you can find open source implementations
---
# Are we done?
--
*  &#x274c; Only supports a single operation - function call operator
---
# How to name operations
--
## Function name
```cpp
  void `rotate`(double degrees);
```
--
  &#x274c; C++17 cannot metaprogram names, except for macros
--
## Type
--
```cpp
  struct `rotate`{};

  using rotate_signature = void(`rotate`,double);
```
--
  &#x2705; C++17 metaprograms types like a boss

---
![fanfare](fanfare.jpg)
---
# Polymorphic
--
## https://github.com/google/cpp-from-the-sky-down/blob/master/metaprogrammed_polymorphism/polymorphic.hpp
---
# Three important things to say about polymorphic
--

### This is not an officially supported Google product.
--
##  This is not an officially supported Google product.
--
#   This is not an officially supported Google product.
---
# Polymorphic combines aspects of virtual and overloading
--
* Specify interface
	* Virtual
--
* Use overloading to connect a type to an operation
	* `poly_extend` is used as the overloaded extension point
---
# Specify Interface
--
## Virtual
```cpp
struct transforming_interface{
  virtual void `translate`(double x, double y) = 0;
  virtual void `rotate`(double degrees) = 0;
};

void spin(transforming_interface& s);
```
--
## Polymorphic
--
```cpp
struct `translate`{};
struct `rotate`{};

void spin(polymorphic::ref<void(`translate`,double x,double y),
                          void(`rotate`,double degrees)> s);
```
---
# Use overloading to connect an interface to an operation
## Overloading
```cpp
template<typename Shape>
void `translate`(Shape& s, double x, double y){
 s.translate(x,y);
}

void `translate`(box& b, double x, double y){
// Translate box.
}
```
--
## Polymorphic
--
```cpp
template<typename Shape>
void poly_extend(`translate`,Shape& s, double x, double y){
 s.translate(x,y);
}

void poly_extend(`translate`,box& b, double x, double y){
// Translate box.
}

```
---
# Spin implementation
--
## Virtual
--
```cpp
void spin(transforming_interface& s){
  s.`translate`(4,2);
  s.`rotate`(45);
}
```
--
## Polymorphic
--
```cpp
void spin(polymorphic::ref<void(translate,double x,double y),
                          void(rotate,double degrees)> s){
  s.`call<translate>`(4,2);
  s.`call<rotate>`(45);
}
```
---
# Using spin
```cpp
class box{
public:
  point upper_left()const;
  void set_upper_left(point);
  point lower_right()const;
  void set_lower_right(point);
  bool overlaps(const box&)const;
};

class circle{
public: 
  void draw() const;
  box get_bounding_box() const;
  void translate(double x, double y);
  void rotate(double degrees);
};
```
--
## Same definitions we used with overload example
--
```cpp
circle c;
box b;
spin(c);
spin(b);
```
--

## &#x2705; There are no polymorphic types, only a polymorphic use of similar types
---
# What about const?
--
```cpp
template<typename Shape>
void smart_draw(`const` Shape& s, const box& viewport){
  auto bounding_box = get_bounding_box(s);
  if(viewport.overlaps(bounding_box)) draw(x);
}
```
---
# Smart Draw
--
## Virtual
--
```cpp
struct drawing_interface{
  virtual void draw() `const` = 0;
  virtual box get_bounding_box() `const` = 0;
};

void smart_draw(`const` drawing_interface& s, const box& viewport){
  auto bounding_box = s.get_bounding_box();
  if(viewport.overlaps(bounding_box)) s.draw();
}
```
--
## Polymorphic
```cpp
struct draw{};
struct get_bounding_box{};

void smart_draw(polymorphic::ref<void(draw)`const`,
                                 box(get_bounding_box)`const`>, const box& viewport){
  auto bounding_box = s.call<get_bounding_box>();
  if(viewport.overlaps(bounding_box)) s.call<draw>();
}
```
---
# Const polymorphic::ref
```cpp
polymorphic::ref<void(draw)`const`, 
                 box(get_bounding_box)`const`>
```
--
## As long as all the function types in a polymorphic::ref are `const` qualified, it will bind to a const object.
---
# Using smart_draw
```cpp
circle c;
smart_draw(std::as_const(c),viewport);

```
---
# Polymorphic object
--
```cpp
using shape = polymorphic::`object`<
                 void(draw)`const`, 
                 box(get_bounding_box)`const`,
                 void(translate,double x,double y),
                 void(rotate,double degrees)>;
```
--
```cpp
shape s1{circle{}};
shape s2 = s1;
spin(s1);
smart_draw(s1,viewport);
smart_draw(s2,viewport);
```
---
# Store in collection
```cpp
std::vector<shape> shapes;
shapes.push_back(circle{});
shapes.push_back(box{});

for(auto& s:shapes) smart_draw(s,viewport);

```
# Value types
```cpp
using shape = polymorphic::object<
                 void(draw)`const`, 
                 box(get_bounding_box)`const`,
                 void(translate,double x,double y),
                 void(rotate,double degrees)>;
std::vector<shape> shapes;
circle c1;
circle c2 = c1;
spin(c1);
spin(c2);

shapes.push_back(c1);
shapes.push_back(c2);
shapes.push_back(box{});

for(auto& s:shapes) smart_draw(s,viewport);

```
---
# Low coupling
---
## With virtual, we had to carefully group our member functions to avoid coupling
--
```cpp
struct drawing_interface{
  virtual void draw() const = 0;
  virtual box get_bounding_box() const = 0;
  virtual ~drawing_interface(){}
};
struct transforming_interface{
  virtual void translate(double x, double y) = 0;
  virtual void rotate(double degrees) = 0;
  virtual ~transforming_interface(){}
};
struct shape: drawing_interface, transforming_interface{};
```
--
## Polymorphic
* Does not matter how you group the function types.
---
# Polymorphic "upcasting"
--
```cpp
void smart_draw(polymorphic::ref<void(`draw`)const,
                                 box(get_bounding_box)const>, const box& viewport);
  auto bounding_box = s.call<get_bounding_box>();
  if(viewport.overlaps(bounding_box)) s.call<draw>();
}
```
--
```cpp
using shape1 = polymorphic::object<
*                void(draw)const, 
                 box(get_bounding_box)const,
                 void(translate,double x,double y),
                 void(rotate,double degrees)>;
shape1 s1{circle{}};
smart_draw(s1,viewport);
```
--
```cpp
using shape2 = polymorphic::object<
                 box(get_bounding_box)const,
                 void(translate,double x,double y),
*                void(draw)const, 
                 void(rotate,double degrees)>;
shape2 s2{circle{}};
smart_draw(s2,viewport);

---
# Benchmarks
* Windows Laptop - i7-8550U
* Fill a vector with different types of polymorphic objects, and then time how long it takes to iterate and call a method on each item.

|Type | Median Time (ns) |
|---|--- |
|Non-Virtual | 121 |
|Virtual | 532 |
|std::function | 539 |
|Polymorphic Ref | 515 |
|Polymorphic Object | 483 |

---
# Size
* `sizeof(ref)` typically `3*sizeof(void*)`
* `sizeof(object)` typically `4*sizeof(void*)`
---

# Final Assessment 
--
*  &#x2705; Low boilerplate
--
*  &#x2705; Easy adaptation of existing class
--
*  &#x2705; Value semantics
--
*  &#x2705; Low coupling
--
*  &#x2705; PPP
--
*  &#x2705; &#x274c; Performance - Inherent overhead in runtime dispatch
--
* &#x2705; Able to be used in non-template functions.
--
* &#x2705; Able to be stored in runtime containers

---
# Show me the code?
--
## https://github.com/google/cpp-from-the-sky-down/blob/master/metaprogrammed_polymorphism/polymorphic.hpp
--
## 248 lines, including comments and whitespace


---

# References
* Sean Parent - Inheritance is the base class of evil - https://www.youtube.com/watch?v=bIhUE5uUFOA
* Louis Dionne - CppCon 2017 - Runtime polymorphism: Back to basics - https://www.youtube.com/watch?v=gVGtNFg4ay0
















































