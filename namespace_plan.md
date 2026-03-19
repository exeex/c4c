# System `<vector>` Support Plan

## Goal

Make `#include <vector>` on the host system standard library fail fast, then
parse progressively further, until a small translation unit that includes
`<vector>` reaches a stable and diagnosable frontier.

This plan is not "implement `std::vector`".
It is a compiler-front-end plan for surviving a real system standard library
header stack.

## Why This Needs Its Own Plan

The existing STL umbrella plan targets STL-like user code written against this
project's currently-supported language subset.

System `<vector>` is a different kind of target:

- it is template-heavy immediately
- it depends on internal type-traits machinery before container code matters
- it stresses parser recovery, dependent names, and header-scale survivability
- it reveals gaps that do not show up in minimal internal smoke tests

So this plan should focus on "real standard-library header ingestion" rather
than container API usability.

## Current Status

Today, a translation unit shaped like:

```cpp
#include <vector>
int main() { std::vector<int> v; }
```

does not complete parsing successfully.

Recent progress has already identified and partially unblocked several early
parser blockers:

- anonymous template parameters
- `extern "C++"`
- bool NTTP specialization syntax
- function-style casts like `bool(x)`
- scoped names in expression position like `Traits::value`
- parser fail-fast protection for header-scale recovery storms

The parse frontier has moved forward, but the header stack still stops early.

## Latest Frontiers From Namespace Work

Recent namespace / using work changed the picture in an important way:

- lexer now recognizes `namespace` / `using`
- parser now has initial top-level support for:
  - namespace definitions
  - `using X = Y;`
  - `using ns::name;`
  - `using namespace ns;`
- internal parse tests now cover:
  - basic / nested / three-level namespaces
  - anonymous namespaces
  - cross-namespace qualified lookup
  - `using` alias / declaration / directive forms

This is enough to show that the next frontier is no longer keyword
recognition.
The harder remaining issue is namespace modeling.

### Important Conclusion

Stringifying names as `a::b::c` is enough to unblock part of the surface, but
it is not a complete design for C++ namespaces.

It becomes fragile around:

- anonymous namespaces
- leading global qualification like `::name` and `::ns::name`
- multi-level namespace interaction
- eventual namespace aliases / inline namespaces
- separating lexical nesting from semantic ownership

The AST / parser state will ultimately need to know the active namespace stack,
not just the flattened final symbol spelling.

### Current Known Failing Cases

All previously failing namespace tests now pass:

- `::some_global_var` and `::oo::a` — fixed: leading `::` in expressions
- `ns::StructType` declarations — fixed: qualified type lookup

Remaining known gap: namespace-qualified function/global names produce `::` in
LLVM IR identifiers, needing quoting or mangling in the emitter.

## Current Codebase Gap Audit

This section captures what the codebase appears to support today versus what
system `<vector>` is likely to require next.

### What Already Exists

- partial `A::B` support in parser type / expression paths
- scoped typedef support for struct members like `StructTag::TypeName`
- template argument parsing in both type and expression contexts
- internal parse regressions for:
  - anonymous template parameters
  - `extern "C++"`
  - `Traits::value` in expression position
  - dependent enum/value forms in template contexts

In other words, the parser is no longer at "no scope resolution at all".
It already has narrow support for qualified names.

### What Is Still Missing In Practice

The next blocker is likely not just "more `A::B` parsing".
It is the absence of real namespace and using-declaration machinery.

Observed gaps in the current codebase:

- namespace handling is still mostly string-based rather than scope-object based
- known-type tracking is mostly flat-string based, not namespace-aware
- leading global qualification `::X` / `::A::B` is not fully supported
- anonymous namespace identity is not modeled as a first-class scope
- current qualified-type recognition is still narrower than full C++ lookup
- parse-only coverage exists for namespace / using forms, but runtime coverage
  still lags behind on the harder namespace cases

That matters because host standard library headers rely heavily on:

- `namespace std { ... }`
- nested internal namespaces
- `using` aliases for traits/plumbing
- `using` declarations that re-export names across namespaces/classes

So the likely frontier is:

- today we can survive some qualified-name syntax
- but we still cannot ingest the declaration forms that make those qualified
  names available in the first place

## Immediate Recommendation

Before pushing deeper on `<vector>` itself, add a dedicated "namespace/using
surface" slice ahead of broader dependent-name work.

Recommended order:

1. finish leading-global qualified lookup support: `::X`, `::A::B`
2. move namespace tracking from flat strings toward explicit scope state
3. make anonymous namespace identity explicit in parser/sema
4. make type-name lookup namespace-aware enough for `std::vector<int>`
5. harden `using` forms against nested / anonymous / multi-level namespace cases
6. only then continue deeper dependent-type-trait work

Without that, `<vector>` progress will likely stay fragile because `std`,
`__gnu_cxx`, and related names cannot be introduced through real source forms.

## Success Criteria

### Phase success

For each phase below:

- add a focused regression test for the exact shape being enabled
- verify that the system `<vector>` smoke case parses further than before
- ensure parse failures remain bounded and do not hang
- once a feature first lands as parse-only coverage, promote the relevant test
  to runtime/front-end coverage before considering that slice complete

### End success

This plan is successful when:

- `#include <vector>` no longer times out in `--parse-only`
- the first remaining failures are clear semantic/instantiation gaps rather
  than parser-recovery pathologies
- we can state a precise frontier such as:
  "header parses, but `std::vector<int>` instantiation still fails on X"
- namespace / using regressions added during this plan are no longer only
  parse-only checks; the intended surviving cases also pass runtime/front-end
  validation

## Scope

### In scope

- parser and sema work needed to ingest system `<vector>`
- dependent-name parsing and lookup scaffolding
- fail-fast and survivability improvements for large standard headers
- targeted internal tests derived from standard-library shapes

### Out of scope

- full standard library implementation
- allocator/runtime behavior parity
- exceptions, RTTI, full `<type_traits>` semantics
- making all of libstdc++ or libc++ compile end-to-end

## Recommended Smoke Case

Use a stable smoke input like:

```cpp
#include <vector>
int main() {
  std::vector<int> v;
  v.push_back(1);
  return (int)v.size();
}
```

Track progress by recording:

- whether parse completes
- the first failure site
- whether the first failure moved forward after each slice

## Work Phases

## Phase 0: Recovery And Observability

Objective:
Make standard-header failures bounded, fast, and easy to localize.

Required slices:

- keep parser error-budget / no-progress guards in place
- add a dedicated regression harness for the `<vector>` smoke file
- record the first failing header/line after each improvement

Exit condition:

- `<vector>` no longer hangs
- failures are reproducible and comparably measurable

## Phase 1: Type-Name Surface Needed By Type Traits

Objective:
Handle the early type-traits layer cleanly.

Likely missing work:

- class/struct names recognized more consistently as type names
- namespace definitions
- `using` alias declarations
- `using` declarations and `using namespace` directives
- leading-global qualification support for `::name` and `::ns::name`
- `typename X::type` in type position
- more robust `A::B` parsing in both type and declaration contexts
- tolerant dependent enum/value forms used by old-style traits machinery

Likely evidence:

- failures around `__true_type`, `__false_type`, `typename ...::__type`

Exit condition:

- `bits/cpp_type_traits.h` parses materially further
- basic namespace-wrapped trait declarations no longer fail at first contact
- namespace tests that first landed as parse-only are promotable to stronger
  runtime/front-end checks

## Phase 2: Dependent Names And Nested Type Access

Objective:
Support the common dependent-name patterns that dominate standard headers.

Required slices:

- parse `typename A::B`
- resolve scoped typedef/alias names in dependent contexts
- make namespace-qualified names survive lookup beyond flat typedef seeding
- stop representing namespace ownership purely as flattened symbol strings
- allow dependent NTTP forwarding in more positions
- separate "known type", "dependent type", and "unknown name" in parser/sema

Representative patterns:

- `typename __truth_type<__value>::__type`
- `typename Traits::value_type`
- `allocator_traits<A>::pointer`

Exit condition:

- parser no longer dies primarily on dependent nested type syntax
- namespace-qualified dependent names are distinguishable from plain unknown ids

## Phase 3: Alias And Trait Plumbing

Objective:
Support the lightweight metaprogramming forms that `<vector>` uses before
container bodies become relevant.

Required slices:

- `using X = Y;`
- alias declarations inside namespace scope
- alias declarations that preserve qualified RHS names
- nested-namespace and anonymous-namespace interaction coverage
- alias templates where needed
- more complete NTTP parsing for identifiers, booleans, chars, and signed values
- better type-context template argument handling

Exit condition:

- early library aliases and trait wrappers parse without falling back to
  "unknown base type"

## Phase 4: Header-Scale Template Survivability

Objective:
Parse larger regions of the library without exploding in recovery or template
bookkeeping.

Required slices:

- better explicit specialization coverage for class templates
- safer handling of dependent constant expressions in parser-time enum contexts
- constructor/type-name ambiguity handling in header-scale code
- progress checks around large recursive template parse paths

Exit condition:

- the `<vector>` include stack reaches container internals rather than failing
  near foundational traits

## Phase 5: `std::vector<int>` Declaration Frontier

Objective:
Move from "header parses further" to "basic named use of `std::vector<int>` is
 understood".

Required slices:

- namespace-qualified template type references remain stable
- declaration of `std::vector<int> v;` reaches sema/HIR frontier cleanly
- remaining failure is narrowed to instantiation/runtime semantics, not parsing

Exit condition:

- we can distinguish parser blockers from true semantic/instantiation blockers

## Known Blockers To Expect

- leading `::` not accepted as the start of a qualified-id
- namespace handling that works only for flattened named paths, not full scope
  identity
- `using` forms not introducing names into parser-visible type tables robustly
- qualified names longer than one `A::B` hop
- parser lookup that depends on pre-seeded typedef strings rather than source
  declarations
- semantic follow-up work after parser support lands, especially around alias
  propagation and namespace-qualified lookup

## Suggested New Regression Tests

Add focused parse-only tests before or alongside implementation:

- `namespace_basic_parse.cpp`
  Contains `namespace std { struct vector_tag {}; }`
- `namespace_nested_parse.cpp`
  Contains nested namespaces and qualified use sites
- `using_alias_basic_parse.cpp`
  Contains `using value_type = int;`
- `using_namespace_alias_parse.cpp`
  Contains `namespace std { template <typename T> struct vector {}; } using Vec = std::vector<int>;`
- `using_declaration_parse.cpp`
  Contains `using std::vector;`
- `using_namespace_directive_parse.cpp`
  Contains `using namespace std; vector<int> v;`

Additional namespace-focused parse tests now matter too:

- multi-level namespace nesting
- anonymous namespace lookup
- cross-namespace qualified references
- leading-global qualification forms like `::name` and `::ns::name`

These should be added as parse regressions first, before attempting a real
system `<vector>` smoke to move again.

- `typename`-qualified dependent nested types
- `using` aliases and alias templates
- richer dependent constant-expression handling
- more complete class-template explicit specialization plumbing
- overload-resolution and constructor matching quality once parsing gets deeper

## Suggested Test Additions

- parse-only: anonymous template param + `extern "C++"`
- parse-only: bool NTTP specialization
- runtime/parse: function-style cast `T(expr)`
- parse-only: scoped-name expression `A::B`
- parse-only: dependent enum initializer placeholder acceptance
- parse-only: type-context NTTP forwarding `Box<N>`
- parse-only: `typename A::B`
- parse-only: `using X = Y`
- parse-only/front-end: dedicated `include_vector_smoke.cpp`
- runtime/front-end promotion target: namespace / using tests should graduate
  from parse-only once the intended semantics stabilize

## Milestone Order

1. keep `<vector>` from hanging
2. make type-traits headers parse further
3. land dependent `typename` / nested-type support
4. land `using` aliases
5. reach a stable `std::vector<int>` declaration frontier

## Notes

- Prefer many tiny parser/sema slices over one large "support stdlib" patch.
- After each slice, re-run the same system `<vector>` smoke case and record the
  new first failure.
- Do not mix this plan with container-runtime behavior work unless the parser
  frontier has already moved into actual `vector` implementation bodies.
