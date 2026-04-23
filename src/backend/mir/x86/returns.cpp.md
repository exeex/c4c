# `returns.cpp` Extraction

## Purpose and Current Responsibility

This file no longer implements x86 return lowering logic. Its current role is a
legacy translation-unit anchor that keeps the old path present while the real
implementation lives under `lowering/return_lowering.cpp`.

The file acts as executable evidence that return lowering was moved out of this
legacy entrypoint and that downstream build references may still expect a
`returns.cpp` unit to exist.

## Important APIs and Contract Surfaces

The only surfaced dependency is the new lowering header:

```cpp
#include "lowering/return_lowering.hpp"
```

The remaining contract is path-level, not algorithmic:

```cpp
// Compatibility placeholder: canonical return lowering now lives in
// `lowering/return_lowering.cpp`.
```

Practical contract:

- compiling this file must not define competing return-lowering logic
- consumers should obtain behavior through the canonical lowering unit, not
  through symbols unique to this file
- the file documents redirection rather than owning semantics

## Dependency Direction and Hidden Inputs

Dependency direction is one-way into the canonical lowering surface:

- `returns.cpp` depends on `lowering/return_lowering.hpp`
- operational behavior is hidden behind the canonical implementation in
  `lowering/return_lowering.cpp`
- build-system history and include/link expectations are implicit inputs, even
  though they are not expressed in code here

Hidden pressure:

- removing this file prematurely may break path-based build assumptions
- leaving real behavior here would create split ownership with the canonical
  lowering path

## Responsibility Buckets

### Compatibility Path

- preserve the historical file location as a stub
- point maintainers at the canonical return-lowering implementation

### Non-Ownership

- no lowering decisions
- no ABI/epilogue policy
- no register or stack return materialization
- no fast-path dispatch

## Fast Paths, Compatibility Paths, and Overfit Pressure

- Core lowering: none in this file
- Optional fast path: none
- Legacy compatibility: the entire file is a compatibility placeholder
- Overfit pressure: high if future edits start reintroducing return logic here
  just to satisfy one caller or build edge instead of fixing the canonical
  lowering path

The main rebuild risk is ownership drift: a tiny shim like this easily becomes
an attractive place for ad hoc fixes because it looks cheap to patch.

## Rebuild Ownership Boundary

This file should own only legacy redirection or disappear entirely once the
build no longer needs the old translation-unit path.

This file should not own return lowering semantics, ABI policy, target-specific
codegen rules, or any new special-case behavior in a rebuild.
