# JavaScript from Beginner to Beginner

## Syntax
* `Infinity - Infinity` -> `NaN`
* `Infinity / Infinity` -> `NaN`
* `NaN == NaN` -> `false`
* `null == undefined` -> `true`
* `null === undefined` -> `false`
* `null == 0` -> `false`
* `null + 4` -> `4`
* `undefined + 4` -> `NaN`
* `1 + '2'` -> `'12'`
* `1 - '2'` -> `-1`
* `false == 0` -> `true`
* `false === 0` -> `false`, use `===` and `!==` to forbid automatic type conversion


## Data
* `var` vs `let`
    * `var` declares variables with **function** scope, `let` declares variables with **block** scope.
    * `let` does not hoist variables (access to it triggers `ReferenceError`) but `var` does (access to it gets `undefined`).
    * duplicate definition with `let` triggers `TypeError`
* function declarations are hoisted, but function expression (anonymous functions like `var func = function () { ... }`) are not
* global variables are attributes of global object `window`
* constants declared with `const` have the same scope as variables declared with `let`
* prototypes
    * `Boolean` `Number` `undefined` `null` `Object` `String` `Symbol`
* convert strings to numbers
    * `parseInt()`, `parseFloat()`
    * unary `+` operator, `Math.abs((+"1.1") + (+"2.2") - 3.3) < 1e8 == true`
* literals
    * array: `var a = ['1st', '2nd', , , '5th, the previous two are undefined', , /* ignored at the end */]`
    * integers: as usual, note bit string can be used: `0b10101110`
    * floating-point
    * object: `var obj = { __proto__: prototype_obj, ['computed' + (() => '!')()]: 123, '!@#$': 'quoted' }` just like JSON
    * regex: `var re = /[a-z]{9}/`
    * string
* false values
    * `false` `undefined` `null` `0` `NaN` `""`
* `Error`
    * used to construct default error objects
    * access error name and message via `.name` and `.message`
* `Promise`
    * 用于实现一些异步操作
    * 分为几种状态：初始态 pending，fulfilled 成功，rejected 失败，settled 或成功或失败（非 pending）
    * **问题**：`var p = new Promise(function (resolve, reject) {});` 这里的 `resolve` 和 `reject` 是不是 `Promise.resolve` 和 `Promise.reject`？
      不知道，但效果是一样的，参见 [http://liubin.org/promises-book/#chapter2-how-to-write-promise](http://liubin.org/promises-book/#chapter2-how-to-write-promise)
* `arguments`
    * access function arguments, can be used to define variadic functions
* default parameter and rest parameter
    * default parameter: `function f(x, y = 1) {}`
    * rest parameter: represent the rest of parameters as an array: `function f(x, ...y) {}`
* arrow functions
    * `x => { ... }` the variable `this` used in arrow functions are interpreted lexically
* `delete`
    * `delete variable_not_defined_with_var`: `ReferenceError` if the deleted variable is accessed again
    * `delete array_name[index]`: produces `undefined` in array
    * `delete object.property`: property would be `undefined` then
* `void (...)`
    * used to specify that the following expression would be evaluated but return nothing
* `X instanceof Y`
    * check if `X` is an instance of `Y`
    * `Object instanceof Object === true`
* `super`
    * `super(...)` call parent constructor
    * `super.function_name(...)` call parent function
* spread operator `...`
    * like Python's unpack, extract multiple things from a single one
    * `var a = [1,2,3]; var b = ['x', ...a, 'y'];`
    * `[..."abc"]` gets `["a", "b", "c"]`
* template literal
    * `var s = \`this is ${Date.now()}\`;` strings containing expressions, like Python `'''` combined with Ruby syntax

## OO-Related
* `Object.keys(obj)` or `Object.getOwnPropertyNames(obj)`
    * list object property names
* `Object.getPrototypeOf(obj)`
    * move up along the prototype chain
* object property
    * a property is just an object which has special members acting as the descriptor, like Python
    * descriptor object members
        * `configurable` type can be changed? property can be deleted?
        * `enumerable` show up when enumerating members?
        * `value` the value
        * `writable` can be written to?
        * `get` getter accessor, a function called when fetching the value
        * `set` setter accessor, a function called when setting the value
    * accessors and `value`, `writable` cannot be used simultaneously
    * use `Object.defineProperty(obj, name, prop)` and `Object.defineProperties(obj, props)` to set
    * object getter/setter
        * `var obj = { a:1, get b() {...}, set c(x) {...} }`
        * accessing `b` and `c` above will call the corresponding functions
* ways to create objects
    * constructor functions like `function Creature(type, hp, mp, skill) { this.type = type; ... }`
    * literal, `var dragon = { type: 'dragon', hp: 30000, mp: 50000, skill: function () { ... } }`
    * `Object.create(prototype, properties)`
* prototype
    * the object that is inherited from
    * `Creature.prototype.name = value` add a new attribute to all `Creature` objects
* `var obj = new Constructor`
    * create a new generic object
    * constructor sets the `__proto__` member to the constructor's `prototype` member
    * pass the object as `this` to the constructor function
    * constructor sets the members

## Prototype Inheritance

```javascript
function Base(a) {
    this.a = a;
}

function Derived(b) {
    Base.call(this, 1); // call Base with this points to the new object
    this.b = b;
}
Derived.prototype = Object.create(Base.prototype);

var d1 = new Derived(2);
var d2 = new Derived(3);

d1.__proto__ === d2.__proto__;      // true
d1.__proto__ === Derived.prototype; // true, can be used as the way to test inheritance relationship
d1 instanceof Derived;              // true, shortcut of the above

Object.getPrototypeOf(d1) === Object.getPrototypeOf(d2); // but use this instead of the magic __proto__

Base.prototype.c = 0; // add a new member to the Base prototype
```

After this, we have:

```
// the prototype of Base
// its prototype, Object, has its own constructor and __proto__ getter/setters internally
// the newly added c = 0 can be accessed along the prototype chain from a Derived object
Base.prototype
  constructor: function Base(a) {...}
  __proto__  : Object
  c          : 0


// this is an "intermediate" object created with the prototype
// being set to Base.prototype
Derived.prototype
  __proto__  : Base.prototype
```

## Iterator && Generator

Iterators are objects with `next` function which returns state objects
containing `done` and `value`.

```javascript
function getIter(max) {
  var current = 0;
  return {
    next: function() {
            return current == max ?
                   {done:true} :
                   {value:++current, done:true};
          }
  };
}
```

Generators uses `function*` to define and can use `yield` to give a value out.

```javascript
function* getFiboGen() {
  var a = 0, b = 1;
  while (true) {
    yield a;
    [a, b] = [b, a + b];
  }
}
```

`yield*` can be used to yield a list of elements:

```javascript
function* gen() {
  yield* ["a", "b", "c"];
}

gen().next(); // gets {value:"a", done:false} ... {value:undefined, done:true}
```

Iterable objects should have a member of `Symbol.iterator` which refers to a no-argument function
producing the iterator or generator object. Those objects can be used with spread expressions and `for..of`

```javascript
var list = {};
list[Symbol.iterator] = function() {
  let k = 0;
  return {
    next: function() {  
            return k < 10 ? {value:k++, done:false} : {done:true};
          }
  };
}
[...list]; // [0, 1, ..., 9]

list[Symbol.iterator] = function() { return getIter(10); } // or reuse the previous getIter
[...list]; // [0, 1, ..., 9]

var fibo = {};
fibo[Symbol.iterator] = getFiboGen;
for (let x of fibo) {
  if (x < 100)
    console.log(x); // 0, 1, 1, 2, ...
  else
    break;
}
````

Generators have `.throw()` method to throw an exception from the suspended `yield`.
If the exception is not captured, it will propagate up along the calling stack. Then
the generator's `done` member will be `true`.

```javascript
function* gen() {
  yield 1;
  try {
    yield 2;
  } catch (s) {
    console.log(s);
  }
  yield 3;
}

var g = gen();
g.throw("hello"); // not captured

g = gen();
g.next(); // get 1, suspend at yield 1
g.throw("hello"); // not captured

g = gen();
g.next(); // get 1, suspend at yield 1
g.next(); // get 2, suspend at yield 2
g.throw("hello"); // captured by the try-catch around yield 2
```

## Meta Programming
### Proxy and Revocable Proxy
Provides hooks (traps) which can be used to customize the behavior of object manipulation.
Unspecified properties are forwarded to the object.

```javascript
var handler = {};
// hook the delete operator
handler.deleteProperty = function(obj, name) { obj[name] = "the new member"; return true; };
var obj = new Proxy({}, handler);
obj.a = 1;
obj.b = 1;
delete obj.c; // this adds a new member obj.c == "the new member"
delete obj.a; // this changes obj.a to the string
```

`var obj = Proxy.revocable({}, handler)` creates a revocable proxy object, with properties:

* `obj.proxy` calls `new Proxy()` and gets the proxy
* `obj.revoke()` turns off the proxy, i.e. the proxy will be unusable, any trap will throw `TypeError`

### `Reflect`
`Reflect`'s static methods provide functions with the same signatures as the proxy traps, but
all in a function (maybe slightly modified) form. For example, `Reflect.construct()` acts as
the function version of `new`.
