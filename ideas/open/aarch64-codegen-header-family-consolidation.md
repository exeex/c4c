# AArch64 Codegen Header Family Consolidation

## Intent

Reduce header count in `src/backend/mir/aarch64/codegen` without undoing the
useful `.cpp` implementation split.

The preferred rule is: `.cpp` files with the same prefix share one family
header. For example, `calls_*.cpp` should include `calls.hpp`, and
`dispatch_*.cpp` should include `dispatch.hpp`.

## Background

The current split created many one-to-one `.cpp` / `.hpp` pairs. That made large
implementation files smaller, but it also made the public/local interface area
look larger than the actual module structure.

The reference prototype has module-level source files such as `calls.rs`,
`memory.rs`, and `emit.rs`; this idea keeps the C++ implementation shards but
moves the headers closer to that module-level mental model.

## Work

Consolidate the local header families first:

- fold `calls_*.hpp` declarations into `calls.hpp`
- update `calls_*.cpp` includes to use `calls.hpp`
- delete now-empty `calls_*.hpp` files
- fold `dispatch_*.hpp` declarations into `dispatch.hpp`
- update `dispatch_*.cpp` includes to use `dispatch.hpp`
- delete now-empty `dispatch_*.hpp` files

After those families are stable, consider the same pattern only for other clear
prefix families.

## Constraints

- Do not merge `.cpp` files in this idea.
- Do not move responsibilities between BIR/prealloc/MIR in this idea.
- Keep declarations grouped by subtopic inside the family header so the larger
  header remains navigable.
- Avoid introducing umbrella includes outside the local family unless required.

## Acceptance

- `calls_*.hpp` and `dispatch_*.hpp` are reduced to the family headers
  `calls.hpp` and `dispatch.hpp`, except for any explicitly justified survivor.
- All affected `.cpp` files include the family header.
- Fresh build proves the include consolidation.
