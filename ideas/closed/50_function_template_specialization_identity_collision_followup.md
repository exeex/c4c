# Function Template Specialization Identity Collision Follow-Up

Status: Closed
Last Updated: 2026-04-09

## Completion Summary

Closed after confirming the underlying specialization-identity fix had already
landed during the just-finished rvalue-reference work and adding an exact-shape
regression for the original `forward_pick<int&>` / `forward_pick<int>` case.

Delivered in this follow-up:

- added `tests/cpp/internal/postive_case/template_forward_pick_specialization_identity.cpp`
  to lock the runtime behavior for both specializations in one TU
- added focused internal HIR and LLVM metadata checks in
  `tests/cpp/internal/InternalTests.cmake` so both
  `forward_pick<T=int&>` and `forward_pick<T=int>` stay materialized with
  distinct specialization metadata
- revalidated the full suite with the regression guard:
  baseline 3162 passing tests, after 3167 passing tests, zero newly failing
  tests

## Leftover Notes

- the exact collision described here is now covered directly, not only through
  the broader `forwarding_ref_qualified_member_dispatch.cpp` regression
- future template-identity work should only reopen as a new idea if a distinct
  specialization shape demonstrates a fresh collision beyond the now-covered
  ref-qualified forwarding path

## Goal

Audit and fix function template instantiation cases where distinct type
bindings collapse onto the same emitted function identity and overwrite one
another in HIR/codegen.

## Why This Idea Exists

While validating the cast-follow-up Step 4 forwarding-reference slice, a wider
template issue surfaced: a single regression that instantiated
`forward_pick<T=int&>` and `forward_pick<T=int>` produced only one emitted HIR
function, so the later specialization silently replaced the other.

That is broader than C-style cast behavior. The cast-specific root cause was
fixed by making ref-overload resolution consult template bindings when scoring
dependent cast arguments, but the specialization-identity collision is a
separate template/materialization problem.

## Example Shape

```cpp
template <class T>
int forward_pick(T&& value) {
  return pick((T&&)value);
}

int main() {
  int x = 0;
  forward_pick<int&>((int&)x);
  forward_pick<int>((int&&)x);
}
```

Observed issue:

- both specializations can lower under the same emitted function name
- only one specialization remains materialized in the module metadata / IR
- runtime behavior then reflects whichever body survived last

## Review Targets

- function template mangling for distinct type bindings
- specialization-key to emitted-name mapping consistency
- deferred instantiation bookkeeping in compile-time/HIR materialization
- module metadata for template specializations

## Success Criteria

- distinct function template bindings materialize as distinct emitted functions
- metadata preserves all instantiated specializations
- a focused regression can instantiate both `T=int&` and `T=int` in one TU
  without one body replacing the other
