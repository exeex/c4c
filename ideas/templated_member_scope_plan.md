# Templated Member Scope Plan

## Goal

Make member declarations inside class templates parse through the normal
function/member declaration pipeline while inheriting the enclosing class
template scope.

This plan exists to avoid two failure modes:

- treating templated-class members as if they were plain non-template members
  with no access to outer template parameters
- treating templated-class members as if they were explicit function templates
  that own those outer template parameters


## Core Position

These two concepts are **not** the same in C++:

1. explicit function template

```cpp
template <typename T>
void f(T x);
```

2. member function of a class template

```cpp
template <typename T, typename Allocator>
struct vector {
  void DoFree(T* p, Allocator* a);
};
```

In case 2, `DoFree` is **not** a function template declaration. It is a normal
member declaration parsed in a context where `T` and `Allocator` are already
visible from the enclosing class template.

So the correct model is:

- semantic model: distinguish explicit function-template scope from inherited
  enclosing class-template scope
- implementation model: reuse the same declaration/function parsing pipeline
  for both, driven by a shared template-scope stack


## Why This Matters

Patterns currently exposed by STL / EASTL bring-up depend on this distinction:

```cpp
template <typename T, typename Allocator>
struct vector {
  typedef unsigned long size_type;
  void set_capacity(size_type n);
  void DoFree(T* p, size_type cap);
};
```

Inside the struct body:

- `T` comes from the enclosing class template
- `Allocator` comes from the enclosing class template
- `size_type` comes from a member typedef in the current class scope

The parser should not need a fresh `template <...>` header on `DoFree` to
recognize those names.


## Design Direction

### 1. Introduce a shared template-scope stack

Add a parser-owned stack describing active template parameter scopes.

Each frame should record at least:

- parameter names
- parameter kind: type vs NTTP
- scope kind:
  - `enclosing_class`
  - `member_template`
  - `free_function_template`
- owning declaration if useful for later matching/debugging

This lets the parser answer:

- is `T` currently a visible type template parameter?
- does `N` name an NTTP in the current parse context?
- did this name come from the enclosing templated class or from the member
  itself?


### 2. Reuse one declaration/function parsing pipeline

Do **not** create a second bespoke parser path for:

- templated-class member functions
- explicit function templates

Instead:

- keep one function/member declaration parser
- run it with different template-scope stacks depending on context

That means:

- free function template:
  - parser pushes a `free_function_template` frame before parsing the function
- templated class member:
  - parser enters the struct body with an `enclosing_class` frame already active
- member function template inside a class template:
  - parser sees both:
    - outer `enclosing_class` frame
    - inner `member_template` frame


### 3. Lookup order should become scope-aware

When parsing types/declarators in class-template member context, lookup should
check in roughly this order:

1. innermost active member-template params
2. enclosing class-template params
3. current class/member typedefs
4. ordinary typedef / namespace-visible names

This is the practical rule needed for cases like:

```cpp
template <typename T, typename Allocator>
struct vector {
  typedef unsigned long size_type;
  void DoFree(T* p, size_type cap);
};
```


### 4. Keep ownership semantics intact

Even though parsing is unified, ownership must remain correct:

- class-template member functions do not own outer class template params
- explicit function templates do own their own template params
- member function templates inside class templates have both layers

This matters later for:

- out-of-class member definitions
- matching declarations to definitions
- overload handling
- specialization / instantiation bookkeeping


## Parser vs HIR Boundary

This work should intentionally preserve a layered split:

### Parser responsibility

The parser should keep template-scope ownership accurate.

At parse time we still need to know whether a visible template name came from:

- an enclosing class template
- a member template
- a free function template

That information is important for:

- correct lookup while parsing declarations
- matching out-of-class definitions back to the right owner
- avoiding the mistake of treating every templated-class member as if it were
  itself an explicit function template

So after parsing, some semantic distinction is still expected to exist.


### HIR responsibility

HIR should normalize function representation as much as possible once parser
scope ownership has already been established.

By the time a member function reaches HIR, we want its representation to make
later passes look uniform:

- complete function parameter list
- resolved return type shape
- owning struct/class when applicable
- explicit template parameters
- inherited template bindings from enclosing class templates

After that normalization, later analysis/lowering should mostly be able to use
the same function pipeline for:

- free functions
- explicit function templates
- members of class templates
- member function templates inside class templates

The distinction should remain available as metadata, not as a reason to keep
forking the entire downstream pipeline.


### Practical rule

Parser keeps the source of template scope precise.

HIR turns that into a normalized function/method form with enough owner and
template-binding metadata that later passes can operate on one common model.


## Example Cases

### Case A: inherited class-template scope

```cpp
template <typename T>
struct Box {
  void push(T value);
};
```

`push` should parse as an ordinary member declaration with access to outer `T`.


### Case B: inherited class-template scope plus member typedef

```cpp
template <typename T, typename A>
struct Vec {
  typedef unsigned long size_type;
  void grow(T* p, size_type n);
};
```

`T` comes from the enclosing template frame; `size_type` comes from current
class scope.


### Case C: nested member-template scope

```cpp
template <typename T>
struct Box {
  template <typename U>
  void assign(T lhs, U rhs);
};
```

The parser should see:

- outer frame: `T`
- inner frame: `U`

without flattening them into one ambiguous bucket.

This shape should be preserved as a standing regression case:

- [templated_member_nested_scope_parse.cpp](/workspaces/c4c/tests/cpp/internal/postive_case/templated_member_nested_scope_parse.cpp)


### Case D: out-of-class member definition

```cpp
template <typename T, typename A>
void Vec<T, A>::grow(T* p, typename Vec<T, A>::size_type n) {}
```

The definition should be matched as a class-template member definition, not as
an unrelated free function template that happens to have the same parameter
names.


## Implementation Sketch

1. add a `template_scope_stack_`-style parser structure
2. push an `enclosing_class` frame when entering a templated struct/class body
3. pop that frame when leaving the body
4. make `is_type_start()` / `parse_base_type()` consult the stack rather than
   only ad hoc sets/maps
5. keep existing member-template injection logic, but represent it as another
   stack frame instead of a disconnected side table when practical
6. update out-of-class qualified member parsing to consult the owning
   templated-class context


## Non-Goal

This plan does **not** claim we should model templated-class members as if they
were explicit function templates.

The goal is:

- one shared parser pipeline
- two different semantic sources of template scope


## Acceptance Criteria

This plan is considered meaningfully implemented once all of the following are
true:

- member declarations inside class templates can use enclosing class template
  parameters without being treated as explicit function templates
- member function templates inside class templates can see both:
  - inherited enclosing-class template params
  - their own explicit member-template params
- out-of-class member definitions retain the correct owner and template-scope
  association
- the following parser regressions stay green:
  - [templated_member_nested_scope_parse.cpp](/workspaces/c4c/tests/cpp/internal/postive_case/templated_member_nested_scope_parse.cpp)
  - [template_member_type_direct_parse.cpp](/workspaces/c4c/tests/cpp/internal/postive_case/template_member_type_direct_parse.cpp)
  - [template_member_type_inherited_parse.cpp](/workspaces/c4c/tests/cpp/internal/postive_case/template_member_type_inherited_parse.cpp)
  - [template_alias_nttp_expr_parse.cpp](/workspaces/c4c/tests/cpp/internal/postive_case/template_alias_nttp_expr_parse.cpp)
  - [template_alias_nttp_expr_inherited_parse.cpp](/workspaces/c4c/tests/cpp/internal/postive_case/template_alias_nttp_expr_inherited_parse.cpp)
  - [member_template_decltype_default_parse.cpp](/workspaces/c4c/tests/cpp/internal/postive_case/member_template_decltype_default_parse.cpp)
  - [member_template_decltype_overload_parse.cpp](/workspaces/c4c/tests/cpp/internal/postive_case/member_template_decltype_overload_parse.cpp)

That keeps the implementation simpler without losing correctness.
