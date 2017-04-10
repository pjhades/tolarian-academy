Reading notes of *Metaprogramming Ruby*

# Object Model
## Open Classes
Class are open and allow users to add/modify methods to/in it.
Classes and modules provide namespaces for names.

When writing libraries, put classes in modules to avoid name clash
with existing ones.


## Relation Between Objects
Attributes related to the object model:

* `.class`: the class of an object
* `.superclass`: the superclass of a class
* `.methods`: methods of an object
* `.instance_methods`: instance methods inherited by objects of a class
* `.private_methods`: private methods of a class
* `.ancestors`: ancestor classes all the way up to the root `BasicObject`
* `.instance_variable_get`: get the value of an instance variable
* `.instance_variable_set`: set the value of an instance variable

## Constants

* `Module#constants` shows all constants in the scope, including class names

* `Module#nesting` shows the `::`-separated names of constant path

* `load('lib.rb')` may introduce constant pollution from `lib.rb`

* `load('lib.rb', false)` creates an anonymous module used as namespace to hold constants from `lib.rb`.
  This is a designed feature since with `load` users want to execute Ruby code, which may pollute the
  current code, but with `require` users want to import a library, so there's no second boolean parameter


## Instance Variables
Only the object's own methods can access its instance variables.

```ruby
class Test
  def initialize
    @a = 1
  end
end

t = Test.new
#  this will call method named :a on object t
t.a
#  access the variables
t.instance_variables
t.instance_variable_get(:@a)
t.instance_variable_set(:@a, 5)
```

To make life easier, use `attr_reader`, `attr_writer`, `attr_accessor`.


## Method Lookup
Starting from the method receiver, follow the ancestor chain to locate the method.

If some class has the form:

```ruby
class Test
  include Mod
end
```

then Ruby **creates an anonymous class that wraps the module** and put it right
above `Test` in the ancestor chain. That's why `Kernel` comes up in the chain.


## `self`
Instance method vs class method:

```ruby
class Test
  # called on some instance object
  # see it in Test.instance_methods
  def instance_method
  end
  
  # called on class object Test itself
  # see it in Test.methods
  def self.class_method
  end
end
```

Private methods can only be called on implicit receivers, the only case is on `self`.


## Class and the Current Class
Classes are just modules.

You may put any executable things in class definitions,
and it **returns the value of the last statement**.

```ruby
result = class MyClass
  self
end

result
```

Besides `self`, Ruby tracks the **current class**, but there's
no way to reference the current class like `self`.

When a method is defined, it becomes the instance method of the current class.
That is to say, if `self` is not a class, the current class is the class of `self`.
That's why methods defined in the top level become the instance methods of `Object`,
since the class of `self`, which is the main object here, is `Object`.

## `class_eval`
Like `instance_eval`, Ruby has `Module#class_eval`, same as `Module#module_eval`,
to allow to execute things w.r.t. a class, even if you don't have a class name
to open it with `class`.

```ruby
def add_method_to(klass)
  klass.class_eval do
    def func(x)
      puts x
    end
  end
end
```

Normally openning a class with `class` keyword  will encounter scope gate,
but `class_eval` is used with a block, thus enjoys the advantages of flat scope.

## Class Instance Variables

Class instance variables are just instance variables owned by
the class object, do not confuse it with instance variables.

```ruby
class MyClass
  @my_var = 1

  def self.read; @my_var; end
  def write; @my_var = 2; end
  def read; @my_var; end
end

obj = MyClass.new
obj.write
obj.read            # => 2
MyClass.read        # => 1
```

## Class Variables
Can be accessed by subclasses and by regular instance methods,
because **class variables are defined w.r.t. class hierarchies**.

```ruby
@@v = 1

class MyClass
  @@v = 2
end

@@v #=>2
```

## Class Name
Classes created with `Class.new` are anonymous and their `name` fields
are `nil`. But if they are assigned to constants like

```ruby
anon = Class.new do
end

anon.name        # => nil
MyClass = anon
anon.name        # => "MyClass"
```

## Eigenclass
Singleton methods are placed in eigenclasses.

* has only one instance (object or class)
* cannot be inherited
* has singleton methods inside


Open the eigenclass like this:

```ruby
class << obj
  # scope of eigenclass
  self
end

class << Klass
  # scope of eigenclass
  self
end
```

`instance_eval` changes `self` of course, and it also changes the eigenclass,
since eigenclasses have only one instance.


Method lookup starts from the object's eigenclass if it has one.
And since eigenclasses are classes, things above them in the inheritance
hierarchy are their superclasses.

The rule is:

* The superclass of the eigenclass of an object is the object’s class.

* The superclass of the eigenclass of a class is the eigenclass of the class’s superclass.

Under this organization, it becomes natural that we can call class methods
defined in superclasses from derived classes.

Since a class object is the instance of it's eigenclass,
"class attributes" can be defined with `attr_xxx` in the eigenclass.

The inheritance hierarchy:

```
                                       Class
                                         ^
                                         |
                                         |.superclass
                                         |
          BasicObject -----.class----> #BasicObject
              ^                          ^
              |                          |
              |.superclass               |.superclass
              |                          |
            Object    -----.class----> #Object
              ^                          ^
              |                          |
              |.superclass               |.superclass
              |                          |
            MyClass   -----.class----> #MyClass
              ^
              |
              |.class
              |
             obj
```

When modules are involved, pay attention to the anonymous class
introduced by including the module:

```ruby
module MyMod
    def f;
        puts 'hello'
    end
end

class Test
    class << self
        include MyMod
    end
end

def get_eigen(klass)
    class << klass
        self
    end
end

t = Test.new
Test.f
p Test.ancestors
eigen = get_eigen(t.class)
p eigen.ancestors

#  result
hello
[Test, Object, Kernel, BasicObject]
[#<Class:Test>, MyMod, #<Class:Object>, #<Class:BasicObject>, Class, Module, Object, Kernel, BasicObject]
```

This is called **class extension**. Similarly you can do **object extension**,
or even use `Object#extend`, which includes a module in some object's
eigenclass:

```ruby
module MyModule
  def my_method; 'hello'; end
end

obj = Object.new
obj.extend MyModule
obj.my_method         # => "hello"

class MyClass
  extend MyModule
end

MyClass.my_method     # => "hello"
```



# Methods
## Dynamic Dispatch
`Object#send` allows to compute the methods you wish to call:

```ruby
obj.send(:method, arg1, arg2)
```

This can even call private methods, use `obj.public_send(:method)` to
respect the object's privacy.

## Dynamic Method
`Module#define_method` allows to define methods dynamically:

```ruby
class Test
  # pass in the method name as argument,
  # and parameters as block variables
  define_method :func do |arg|
    arg * 3
  end
end
```

`define_method` is called in class definition, so the `self` is
the class object itself (`Test` here).

## `method_missing()`
`Kernel#method_missing` is called when Ruby cannot find the invoked method.
`method_missing()` is private, so call it by `obj.send(:method_missing, :non_existing_method)`.
Find it with `Kernel.private_methods.grep /missing/`.

With `method_missing`, we can create **Ghost Methods**, methods that can
be called but not actually exist, to do something like intercepting the method
call and delegating the call to some existing method (**Dynamic Proxy**).

Use `delegate` library to create proxy objects.

`Module#const_missing` is called when a an undefined constant is referenced
in a module object, like `Kernel`, where the constant name is passed in as a symbol.

## Blank Slate
A class with some inherited methods removed, in order to
prevent the ghost methods from failing because of method name clash.

Two ways to remove methods:

* `Module#undef_method` **prevents any call** to a certain method, throwing `NoMethodError`

* `Module#remove_method` **removes** a certain method from the receiver, but method searching will
   still be performed on later calls

Leave alone the reserved methods like `Object#__id__` and `Object#__send__` to avoid warning.

## Alias
Use `alias` keyword or `Module#alias_method` to give a method a new name. This is just a name-to-method mapping,
so if any name is assigned to a newly defined method, other names won't be
affected.

```ruby
class String
  alias :real_length :length

  def length
    real_length > 5 ? 'long' : 'short'
  end
end

"War and Peace".length # => "long"
"War and Peace".real_length # => 13
```

**Around alias** is a pattern that you use alias to modify the behavior
of some method, and delegate the call to its original form in some cases.
This is a kind of monkeypatching, and can break existing code. Usually
the class involving around alias should not be loaded more than once,
since the `alias` will be applied again on the name we renamed previously.

## Hook Method

Executed when a certain events occurs:

* `Class#inherited()`
* `Module#included()`
* `Module#method_added()`
* `Module#method_removed()`
* `Module#method_undefined()`
* `Kernel#singleton_method_added()`
* `Kernel#singleton_method_removed()`
* `Kernel#singleton_method_undefined()`

With the help of `Module#included` hook, you can implement **class extension mixins**:

```ruby
module MyMixin
  def self.included(base)
    # add to the eigenclass of a class
    # thus making them class methods
    base.extend(ClassMethods)
  end

  # things to be used to extend
  module ClassMethods
    def x
      "x()"
    end
  end
end
```

Besides using the `included` hook, you may also override `Module#include`,
the `include` class macro. But make sure to call the base implementation with `super`,
otherwise the event can be intercepted but the including will not be done:

```ruby
module M
end

class C
  def self.include(*modules)
    puts "Called: C.include(#{modules})"
    super
  end
  include M
end

#  run it
Called: C.include(M)
```



# Block
## Scope
Ruby has no lexical scope, so in class/model/method definition you cannot
access variables defined outside. But blocks are as closures therefore
code in blocks can access variables outside of it.

**Scope gates** flags the creation of new scopes:

* `class` class
* `module` module
* `def` method

**Flattened scopes** allow us to access outside things inside class/module/method definitions.

* `Class#new`
* `Module#new`
* `Module#define_method`

```ruby
var = 1
MyClass = Class.new do
  puts var
  # class defnition here
end
```

**Shared scope** allow us to share variables among definitions, thus providing
a guard:

```ruby
def define_shared_scope
  shared = 0

  Kernel.send :define_method, :counter do
    shared
  end

  Kernel.send :define_method, :inc do |x|
    shared += x
  end
end

define_shared_scope
counter
inc 5
counter
```
## `instance_eval()` and `instance_exec()`
`Object#instance_eval` provides **Context Probe**, which allows
you to execute code w.r.t. an object but also as in a block.

```ruby
v = 2
obj.instance_eval { @v = v }
obj.instance_eval { @v }
```

`Object#instance_exec` is the same as `instance_eval` but allows
parameters to be used in the block.

This will break encapsulation thus should be used carefully.

A **clean room** is an object with instance methods and let
you execute blocks w.r.t. it.

## Callables
Blocks are a piece of code ready to be executed later, but
they are **NOT** objects.

To carry blocks around, use `&` operator, and provide
the **last parameter** in method definitions prefixed by `&`.
This lets you pass the block given to one method to another method.

```ruby
def my_method(greeting)
  puts "#{greeting}, #{yield}!"
end

my_proc = proc { "Bill" }
my_method("Hello", &my_proc)
```

Ruby has several kinds of callables that also provide **deferred evaluation**:

* `Proc`
  - created with `Proc.new` or `proc()`

  - called with `Proc#call`

  - `return` jumps out from **the scope where the `Proc` is defined**

* lambdas
  - is actually `Proc`

  - created with kernel method `lambda()`

  - or with **stubby lambda** `plus_one = ->(x) { x + 1 }`

  - `return` jumps out from **the scope of the callable itself**

  - are more strict than `Proc`s

* methods
  - evaluated in the scope of the object

  - obtain the corresponding `Method` object with `Object#method`: `m = obj.method :my_method; m.call`

  - unbind with `Method#unbind`, returning an `UnboundMethod` object

  - rebind with `UnboundMethod#bind`, but the target object should **have the same class** as the one from with the method was unbound

  - can be converted to `Proc` with `Method#to_proc`


## Singleton Methods

Defined on a single object.

List singleton methods with

```ruby
obj.singleton_methods.grep /^my_method/
m = obj.singleton_method :method_name
```

Class methods are just singleton methods of the class object,
which are held in the class object's eigenclass.

Methods like `Module#attr_xxx` are called **class macros** since they
behave just like keywords.


# Misc
## `Kernel#eval` and Binding

`Kernel#eval` executes code provided in a string.

```ruby
eval(code, binding, file, line)
```

Binding is **an object of class `Binding`** which records
the **local scope** and allows you to later execute code
w.r.t. it.

Capture the local binding with `Kernel#binding` method.
Capture the top-level binding with `TOPLEVEL_BINDING` constant.

```ruby
class MyClass
  def my_method
    @x = 1
    binding
  end
end

#  now b has the local binding inside my_method
b = MyClass.new.my_method

#  execute code w.r.t. a given binding
eval "@x", b  # => 1

class AnotherClass
  def my_method
    eval "self", TOPLEVEL_BINDING
  end
end

#  now the eval in my_method will run against top-level binding
AnotherClass.new.my_method    # => main
```

Ruby REPLs like irb and pry can create **nested sessions** w.r.t. a specific object:

```ruby
#  create a string object
[66] pry(main)> s = "abc"
=> "abc"
#  now start a new pry session w.r.t. to it
[67] pry(main)> pry s
#  the prompt has changed
[1] pry("abc")> self
=> "abc"
[2] pry("abc")> reverse
=> "cba"
[3] pry("abc")> length
=> 3
[4] pry("abc")> exit
=> nil
#  return to the root session
[68] pry(main)> self
=> main
```

Security tips about `eval`

* Use `obj.tainted?` to check if an object is potentially unsafe
* Use `$SAFE` to set **safe levels** to limit the behavior of code
