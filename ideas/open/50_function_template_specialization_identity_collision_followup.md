# Function Template Specialization Identity Collision Follow-Up

Status: Open
Last Updated: 2026-04-09

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
