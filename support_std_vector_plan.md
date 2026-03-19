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

## Success Criteria

### Phase success

For each phase below:

- add a focused regression test for the exact shape being enabled
- verify that the system `<vector>` smoke case parses further than before
- ensure parse failures remain bounded and do not hang

### End success

This plan is successful when:

- `#include <vector>` no longer times out in `--parse-only`
- the first remaining failures are clear semantic/instantiation gaps rather
  than parser-recovery pathologies
- we can state a precise frontier such as:
  "header parses, but `std::vector<int>` instantiation still fails on X"

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
- `using` alias declarations
- `typename X::type` in type position
- more robust `A::B` parsing in both type and declaration contexts
- tolerant dependent enum/value forms used by old-style traits machinery

Likely evidence:

- failures around `__true_type`, `__false_type`, `typename ...::__type`

Exit condition:

- `bits/cpp_type_traits.h` parses materially further

## Phase 2: Dependent Names And Nested Type Access

Objective:
Support the common dependent-name patterns that dominate standard headers.

Required slices:

- parse `typename A::B`
- resolve scoped typedef/alias names in dependent contexts
- allow dependent NTTP forwarding in more positions
- separate "known type", "dependent type", and "unknown name" in parser/sema

Representative patterns:

- `typename __truth_type<__value>::__type`
- `typename Traits::value_type`
- `allocator_traits<A>::pointer`

Exit condition:

- parser no longer dies primarily on dependent nested type syntax

## Phase 3: Alias And Trait Plumbing

Objective:
Support the lightweight metaprogramming forms that `<vector>` uses before
container bodies become relevant.

Required slices:

- `using X = Y;`
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
