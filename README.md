<div align="center">

# Salmon Programming Language
</div>

<div align="center">
    <img alt="GitHub open issues" src="https://img.shields.io/github/issues-raw/Salmon-lang/Salmon?style=for-the-badge">
    <img alt="GitHub closed issues" src="https://img.shields.io/github/issues-closed/Salmon-lang/Salmon?style=for-the-badge">
    <img alt="GitHub open pull requests" src="https://img.shields.io/github/issues-pr/Salmon-lang/Salmon?style=for-the-badge">
    <img alt="GitHub closed pull requests" src="https://img.shields.io/github/issues-pr-closed/Salmon-lang/Salmon?style=for-the-badge">
    <img alt="GitHub forks" src="https://img.shields.io/github/forks/Salmon-lang/Salmon?style=for-the-badge">
    <img alt="GitHub stars" src="https://img.shields.io/github/stars/Salmon-lang/Salmon?style=for-the-badge"> 
    <img alt="Version" src="https://img.shields.io/badge/release-v0.5-%23ff0000?style=for-the-badge">
</div>

<div align="center">

---
### Overview
<br>
</div>

Salmon is a dynamically-typed, garbage-collected programming language designed to support Object-Oriented Programming and First-Class Functions.

<div align="center">

---
### Dependencies
<br>
</div>

To successfully build Salmon, the following dependencies are required:
- CMake version 3.14 or higher
- Any C compiler

<div align="center">

---
### Building Salmon
<br>
</div>

1. Clone the repository:
   ```bash
   git clone https://github.com/Salmon-lang/Salmon.git
   ```
2. Navigate to `common.h` and adjust the flags to enable or disable debugging and ensure compatiblity with your machine.

3. run the following commands
    ```bash
    cmake -B bld
    cmake --build bld
    ```
5. If you get any errors, relpace `NAN_BOXING` with `_NAN_BOXING` in `common.h`

<div align="center">

---
### Use
<br>
</div>

To use Salmon, run `./bld/salmon` followed by the path to the `.salmon` file you plan to run.
<br><br>

<div align="center">

## Docs
### Data Types
</div>

Salmon has seven primative data types:
- double
    - Represents an 64-bit double-presision floating-point number.
- booleans
    - Can either be `true` or `false`
- strings
    - A sequence of characters; supports easy concatonation.
- nil
    - Represents the absence of a value; default for variables and function returns.
- array
    - A list of values that can be accessed and modified.
- closures
    - Functions treaded as values; allowing for dynamic usage.
- objects
    - Complex structures with methods and variables.

---
<div align="center">

### Mathematical Operators
</div>

Salmon has 4 mathematical operators. They are:
- `+`:Addition, string concationation, or appending to an array.
- `-`:Subtractions or negation.
- `*`:Multiplication.
- `/`:Division.

---
<div align="center">

### Variables
</div>

Salmon, being dynamically typed, allows variables to hold any value such as doubles, strings, booleans, or other types.

#### Declaration

Unlike some languages, Salmon requires explicit variable declaration using the `var` keyword followed by the variable name.

#### Asignment
To assign a value, use `:=`. Other assignment operators like `+=`, `-=`, `*=`, `\=` perform mathematical operations and update the variable.
```salmon
var a; // Declared and set to nil
var b := 2; // Declared and set to 2
a := 3; // Set to 3
```
---
<div align="center">

### Conditionals 
</div>

Conditional statements in Salmon evaluate boolean expressions and execute code based on the result.

#### Booleans
There are five boolean operators:
- `<`, `>`, `=`, `<=`, `>=`:Comparison operators.
- `!`:Negation.
- `&`, `|`:Logical AND and OR.

All values have a boolean value, Strings, objects, closures, arrays, non-zero numbers, and `true` are truthy, while `nil`, `0`, and `false` are falsey.
#### if Statements
```salmon
var a := true;
var b;

if (a = 2) {
    b := 12;
} else if (a = "true") {
    b := "hi";
} else {
    b := 0;
}

if (a & b)
    // Do something; no need for curly braces
    // if only one statement in the if block

b := "hi" & nil; // Sets b to nil because the left, "hi" is truthy
```
#### ternary operators
```salmon
var x := (1 + 1 = 2) ? 2 : 1; // Sets x to 2 because 1 + 1 is equal to 2
```
#### while loops
```salmon
var i := 0;
var j := 0;

while (i < 3) {
    i += 1;
    j += i;
}

while (j)
    j -= 1; // The code in the while block does not need
            // curly braces if it is just one statement
```
#### for loops
```Salmon
var j := 0;

for (var i := 0; i < 3; i += 3)
    j += i;
```
---
<div align = "center">

### Arrays
</div>

Arrays in Salmon are declared using `[]`. Elements can be accessed, set, and values can be appended.
#### Declaration
```salmon
var my_array := []; // Declares an empty array
```
#### Accessing an Element
```salmon
var value := my_array[0]; // Access the first element of the array
```
#### Setting an Element
```salmon
my_array[0] := 42; // Sets the first element of the array to 42
```
#### Appending values
```salmon
my_array += 99; // Appends 99 to the end of the array;
```
---
<div align="center">

### Functions
</div>

#### Function Definition
Functions in Salmon are defined using the `function` keyword, followed by the name, parameters, and body.
```salmon
function add(a, b) {
    return a + b;
}
```

#### Function Calls
Functions are called by typing the name followed by the parameters.
```salmon
var result := add(3, 5);
```
#### Native Functions
Native functions are pre-defined in C and indecated by a preceding `_`.
```salmon
function print(contents) {
    _print(contents); // Prints contents to the console
}

print("Hell0, World!");
```

#### Closures
Closures are functions treated as values. They can be passed around, called, and assigned to variables.
```salmon
function greet() {
    _print("Greetings!");
}

var my_closure := greet;
my_closure(); // Calls the closure
```

#### Lambdas
Lambdas are anonymous functions. They are treated as a closure.
```salmon
function apply(x, fn) {
    return fn(x);
}

var x := 12;
var square := |x| => {
    return x * x;
}; // Creates the lambda and assigns it to the variable 'square'

x := apply(x, square);
```
---
<div align="center">

### Object-Oriented Programming (OOP)
</div>

#### Classes
Classes are defined with the `class` keyword, followed by the name and body.
```salmon
class Person {
    say_hello(name) {
        _print("Hello, my name is ");
        _print(name);
    }
}
```
#### Objects
Objects are instances of classes and can be created by using the class anem followed by parameters.
```salmon
var person := Person();
person.say_hello("John"); // Outputs "Hello, my name is John"
```
#### Methods
Methods are functions associated with a class. They can be called from a object using `.`;
```salmon
class Calulator {
    add(a, b) {
        return a + b;
    }
}

var calculator := Calulator();
var sum := calculator.add(3, 4); // Calls the add method
```
#### Private Methods
Private methods are functions associated with a class. They can only be called inside of the class and subclass.
```salmon
class A {
    private hello(name) {
        _print("hello ");
        _print(name);
    }

    say_hello(name) {
        this.hello(name); // Calls the private method 'hello'
    }
}

var a := A();
a.say_hello("John"); // Calls the say_hello method
a.hello("John"); // Errors because hello is private
```
#### Variables
Class variables are declared by using `this.` in the class, and the can be accessed in objects using `.`.
```salmon
class Circle {
    get_area() {
        return 3.1415926 * this.radius * this.radius;
    }
}

var my_circle = Circle();
my_circle := 5;
var area := my_circle.get_area();
```
#### Initialization
The `init` function is a special method called when creating an object.
```salmon
class Car {
    init(brand) {
        this.brand := brand;
    }
}

var my_car := Car("Toyota");
```
#### Inheritance
To inherit from a superclass, use `<` between the class name and the superclass name.
```salmon
class Animal {
    speak() {
        _print("Animal speaks");
    }
}

class Dog < Animal {
    speak() {
        _print("Dog barks");
    }
}

var my_dog := Dog();
my_dog.speak(); // Outputs "Dog barks"
```
#### Super
`super` is used to reference the superclass inside a class.
```salmon
class Base {
    greet() {
        _print("Greetings from the base class");
    }
}

class Derived < Base {
    greet() {
        super.greet();
        _print("Additional greetings from the derived class");
    }
}

var my_object := Derived();
my_object.greet();
```
---
<div align="center">

### Conclusion
</div>

Salmon offers a versitile and expressive programming environment, facilitating a wide range of programming paradimes. Whether engaging in Object-Oriented Programming, utilizing First-Class Functions, or exploring the intracacies of dynamic typing, Salmon provides a frexible and powerful platform for software development.
