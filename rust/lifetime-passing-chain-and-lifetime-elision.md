# Lifetime Passing Chain and Lifetime Elision

## Problematic Code

```rust
use std::cell::{Ref, RefCell};
use std::ops::Deref;

#[derive(Debug)]
struct Whatever;

#[derive(Debug)]
struct Inside {
    rf: Option<RefCell<Whatever>>
}

#[derive(Debug)]
struct Wrapper {
    inside: Inside
}

impl Deref for Wrapper {
    type Target = Inside;

    fn deref(&self) -> &Self::Target {
        &self.inside
    }
}

fn main() {
    let wrapper = Some(Wrapper { inside: Inside { rf: Some(RefCell::new(Whatever))}});
    let mut outside: Option<Ref<'_, Whatever>> = None;
    wrapper.map(|w| {
        outside = w.rf.as_ref().map(|x| x.borrow())
    });
}
```

This results in:

```
error[E0521]: borrowed data escapes outside of closure
  --> src/main.rs:29:9
   |
27 |     let mut outside: Option<Ref<'_, Whatever>> = None;
   |         ----------- `outside` declared here, outside of the closure body
28 |     wrapper.map(|w| {
29 |         outside = w.rf.as_ref().map(|x| x.borrow())
   |         ^^^^^^^   - borrow is only valid in the closure body
   |         |
   |         reference to `w` escapes the closure body here
```

If we desugar this line:

```rust
outside = w.rf.as_ref().map(|x| x.borrow())
```

we get

```rust
                                      // take ownership of w
                                      // which is valid within the closure
outside = w
    .deref()                          // borrows w as &'a Wrapper
                                      // gives &'a Inside
    .rf                               // gives Option<RefCell<Whatever>>
    .as_ref()                         // borrows the above Option as &'b Option<RefCell<Whatever>>
                                      // gives Option<&'b RefCell<Whatever>>
    .map(|x: &'b RefCell<Whatever>| {
        x.borrow()                    // called on &'b RefCell<Whatever>
    })
```

Now what did we do with `'b`? We borrowed the `rf` field of a `&'a Inside`.
So this borrowed `rf` cannot outlive the `&'a Inside` and `'a: 'b` should hold.
Thus if this code compiled, the `Ref` we assigned to `outside` would outlive
the closure, which violates the lifetime constraint.

## Take-aways

- Generic lifetimes are like generic types: depending on the actual situation,
  different actual types can be plugged into a generic type `T`; depending on
  the actual situation, different actual lifetimes can be plugged into a generic
  lifetime `'a`.
- Lifetime bounds like `'a: 'b` means `'a` must be longer than `'b`, which seems
  counterintuitive at first sight. But it make sense: when we talk about traits,
  `A: B` tells you that something that is `A` is also `B`. It says that `A` is
  somehow _subjected to_ `B`. Now for `'a: 'b`, `'a` outliving `'b` means that
  when `'b` is valid, `'a` must be valid too, because it's longer. So in this sense
  `'a` is also _subjected to_ `'b`, where the reverse does not hold.
- For functions like `fn (&self)`, the lifetime of `self` may change from one call
  to another, because it depends on the actual object with which the function is
  called.
