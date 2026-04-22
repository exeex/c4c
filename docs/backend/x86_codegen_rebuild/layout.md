# X86 Codegen Rebuild Layout

This file records the reviewed ownership buckets for the replacement draft
tree. Stage 3 should fill file contents without renaming, merging, or dropping
these artifacts.

## Ownership Direction

- `api/`: public x86 codegen entrypoints only
- `core/`: shared types and output helpers consumed by canonical lowering
- `abi/`: target profile, register naming, and ABI policy facts
- `module/`: top-level module orchestration and module-data emission
- `lowering/`: canonical semantic lowering families
- `prepared/`: prepared-route query adaptation and thin fast-path selection
- `debug/`: route-proof and route-summary surfaces only

## Route Constraints

- prepared files stay consumers of canonical frame, call, memory, comparison,
  and output seams
- module/data emission stays separate from fast-path dispatch
- debug files summarize route facts and do not become hidden lowering helpers
- compatibility facts must be called out explicitly instead of being hidden in
  generic helpers
- the replacement scope excludes the `peephole/` subtree unless lifecycle
  state expands the boundary later

## Manifest Summary

- Headers: 18 mandatory `.hpp.md` artifacts
- Implementations: 16 mandatory `.cpp.md` artifacts
- Top-level review artifacts: `index.md`, `layout.md`, `review.md`

## Next Drafting Order

1. `api`, `core`, `abi`, and `module`
2. canonical `lowering`
3. `prepared` and `debug`
4. final coherence review in `review.md`
