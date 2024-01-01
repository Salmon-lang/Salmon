<div align="center">

# Salmon
</div>
<div align="center">
    <img alt="GitHub forks" src="https://img.shields.io/github/forks/Salmon-lang/Salmon?style=for-the-badge">
    <img alt="GitHub stars" src="https://img.shields.io/github/stars/Salmon-lang/Salmon?style=for-the-badge"> 
    <img alt="Version" src="https://img.shields.io/badge/release-v0.1-%23ff0000?style=for-the-badge">
</div>

<div align="center">

---
### What is Salmon?
<br>
</div>

Salmon is a dynamically typed, garbage collected language. It supports:
- Object Oriented Programming
- First Class Functions

<div align="center">

---
### Dependencies
<br>
</div>

- cmake v3.14+
- any c compiler

<div align="center">

---
### Build
<br>
</div>

1. clone the repo
```bash
git clone https://github.com/Salmon-lang/Salmon.git
```
2. go into `common.h` and edit the flags to turn on and off debugging and to ensure that it runs on your machine

3. run
```bash
cmake -B bld
```
4. run
```bash
cmake --build bld
```
5. If you get any errors, relpace `NAN_BOXING` with `_NAN_BOXING` in `common.h`

<div align="center">

---
### Use
<br>
</div>

To use Salmon, run `./bld/salmon` plus the path to the `.salmon` file you plan to run.
<br><br>

<div align="center">

## Docs
### Data Types
</div>

Salmon has 6 primative data types. They are:
- double
    - A number in Salmon is stored as a 64 bit, double persision, floating point decimal
- booleans
    - A boolean can have one of two states, `true` or `false`
- strings
    - A string is just a bunch of text. They can easily be concatonated. Salmon also treats all whitespace inbetween the opening and closing quotes of a string as part of the string, this includes spaces, tabs, and newlines
- nil
    - `nil`is just the default value of variable and returns of functions, `nil` represents no value
- closures
    - closures are to complicated for a bullet point, they will get their own section
- objects
    - objects are to complicated for just a bullet point, they will get their own section

---
<div align="center">

### Mathematical Operators
</div>

Salmon has 4 mathematical operators. They are:
- `+`
    - add two numbers together or concatonates two strings together
- `-`
    - subtracts two numbers, the right number from the left
- `*`
    - multiplies one number by another
- `/`
    - divides two numbers, the left by the right

---
<div align="center">

### Variables
</div>

As stated before, Salmon is a dynamically typed language, that means that a variable can hold any value, be it a double, a string, a bool, or any other type.

#### Declaration

Unlike python, Salmon requires a variable to be explicetally declared before anything can be done with it. To declare a varibale the keyword `var` is used followed by the name of the variable.

#### Asignment
Inorder to really do anything with a variable, it must first have a value. In order to asign a value to a variable, `:=` is used followed by the value that you want to set the variable to. `+=`, `-=`, `*=`, `/=` are all other ways of asignment, they perform the mathematical operation on the original value of the variable with the value on the right and asigns the new value to the variable. A declaration and an asignment can be in the same statement. If there is not expicit asignment, a decalred variable it set as `nil`.

```salmon
var a; // a is declared and set to nil
var b := 2; // b is declared and set to 2
a := 3; // a is set to 3
```
---
<div align="center">

### Conditionals 
</div>

All a conditional does is it takes in a boolean expression and does something if it is `true` or `false`.

#### Booleans
There are 5 boolean operators:
- `<`
    - returns `true` if the left is less than the right, otherwise, `false`
- `>`
    - returns `true` if the left is greater than the right, otherwise, `false`
- `=`
    - returns `true` if the left is equal to the right, otherwise, `false`. Works with non numbers as well
- `<=`
    - returns `true` if the left is not greater than the right, otherwise, `false`
- `>=`
    - returns `true` if the left is not less than the right, otherwise, `false`
- `!`
    - returns the iverse boolean value of whatever the input includes
- `&`
    - if the left is `true` returns the right, otherwise, returns the left
- `|`
    - if the left if `false` returns the right, otherwise, returns the left

All values have a boolean value. All strings, objects, closures, numbers besides `0`, and `true` have a `true` boolean value, while `nil`, `0`, and `false` all have a `false` boolean value
#### if statements
`if` takes in a boolean expression, and if it evaluate as `true`, it executes the block attached, if there is an `else` section attached to the `if`, it will execute the `else` block if the boolean evaluates `false`. You can even combine them into an `else if`
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
    //do something, do not need curly braces if only one statement in if block

b := "hi" & nil; //sets b to nil because the left, "hi", is truethy so & returns the right, nil
a := 1 | 3; //sets a to 1 because the left, 1, is truethy so | returns left, not right
```
#### while loops
`while` takes in a boolean expression, and will execute the code inside of the block until the boolean expression evaluates as `false`
```salmon
var i := 0;
var j := 0;
while (i < 3) {
    i += 1;
    j += i;
}

while (j)
    j -= 1; //the code in the while block does not need curly braces if it is just one statement
```
#### for loops
`for` takes in an optional variable declaration/asignment, and optional boolean expression, and an optional update section. It first asigns/declares the variables in the first section, then check if the boolean expression evaluates as `true`, then it executes the code in the `for` block, and finally it does the update section. It will repeate the `for` block execution and update section until the boolean expression evaluates `false`
```Salmon
var j := 0;
for (var i := 0; i < 3; i += 3)
    j += i;
```
---
<div align="center">

### Functions
</div>

#### function definition
A function is defined using the keyword `function` followed by a name and the `(` the parameters `)` and finally `{` the body `}`. If there is a plain `return` or none at all, the function will return `nil`
```salmon
function foo(bar) {
    return bar + 1;
}
```

#### function calls
A function is called by typing the name followed by `(` the paramters `)`. A function can not be called before it is defined, it can be called before it is declared, but the function must have been already defined at that point in the program
```salmon
function call() {}
call();

function foo() {
    bar();
}
foo(); // not ok, bar has not been defined
function bar() {}
foo(); // ok, bar has been defined
```

#### native functions
Native functions are functions that have been defined in c. They can bee spoted by their preceeding `_`
```salmon
function print(contents) {
    _print(contents); //prints contents to the console
}

print("Hell0, World!");
```

#### closures
Closures are the functions as values, they can be passed around, called, and asigned to variables. They are used by writing out the function name or variable that holds it with out the `()` and parameters
```salmon
function foo() {
    _print("foo");
}

foo(); // prints foo
var bar := foo;
bar(); //prints foo
```
---
<div align="center">

### OOP
</div>

#### classes
A class is defined with the keyword `class`, follwed by the name and `{` the body `}`.
#### objects
An object is just an instance of the class that has been created. In order to create an object, just type the name of the object as well as `(` the parameters `)`
#### methods
Methods are just functions attached to a class/objects. They can be called from a object using `.` to access any feild, be it a method or a variable. They are declared just like a function, the only difference it the lack of the `function` keyword
#### variables
Variables in a class/object are declared in a class using `this.` followed by the name of the variable. In an object they can be accesed uint `.`
#### this
`this` is a way to reference the class inside of the same class
#### init
`init` is a special function. It is called when creating an object
#### inheritance
To inherit from a superclass, `<` is used inbetween the class name and the superclass name. All methods are inherited from the superclass, they can be redefined
#### super
`super` is used the same as `this`, just for the superclass of the class.
```salmon
class Foo {
    init() {}
    a() {
        _print("foo");
    }
    b() {
        _print("b");
    }
}

class Bar < Foo {
    init() {
        super.init();
        this.bar = "bar";
    }
    a() {
        super.a();
        _print(this.bar);
    }
}

var bar := Bar();
bar.a();
bar.b();
```
