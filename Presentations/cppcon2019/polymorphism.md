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
# &#x2705
---
# What about box

```cpp
box b;
smart_draw(b,viewport);
```

--
# &#x274c 

---
# What to do

Write a facade?
--

# &#x274c 
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




