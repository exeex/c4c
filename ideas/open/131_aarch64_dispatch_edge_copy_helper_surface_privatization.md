# AArch64 Dispatch Edge-Copy Helper Surface Privatization

## Goal

Privatize or fold back the AArch64 dispatch edge-copy helper surface that is
currently public but only used by dispatch-family code.

## Why This Exists

The post-contract dispatch-family audit found that
`dispatch_edge_copies.hpp` exposes helper types and functions whose direct use
is limited to `dispatch_edge_copies.cpp` or nearby `dispatch.cpp` flow. The
remaining behavior is target-local edge-copy emission and dispatch
organization, not a stable external public hook.

This idea exists to narrow that surface without changing edge-copy behavior.

## In Scope

- `src/backend/mir/aarch64/codegen/dispatch_edge_copies.hpp`
- `src/backend/mir/aarch64/codegen/dispatch_edge_copies.cpp`
- `src/backend/mir/aarch64/codegen/dispatch.cpp`
- `src/backend/CMakeLists.txt` only if a translation unit is removed
- Privatizing dispatch-family-only helper declarations
- Folding helper code into `dispatch.cpp` only where that owner is clearer

## Out Of Scope

- Changing edge-copy instruction order
- Changing predecessor select parallel-copy behavior
- Changing block-entry publication behavior
- Replacing prepared edge-copy or publication facts with local rediscovery
- Moving AArch64 register hazard policy or final instruction spelling into
  shared code

## Acceptance Criteria

- Public declarations in `dispatch_edge_copies.hpp` are limited to real
  cross-file hooks, or the header is removed if no such hooks remain.
- Edge-copy emission order, predecessor select parallel-copy lowering, and
  block-entry publication behavior are preserved.
- Any file-count reduction follows from removed public surface or clearer
  ownership, not from mechanical movement alone.
- Build metadata is updated only if the translation-unit set changes.

## Proof Route

- `cmake --build --preset default`
- `ctest --test-dir build -R '^backend_aarch64_' --output-on-failure`

Use matching `test_before.log` and `test_after.log` if public header or
translation-unit shape changes.

## Reviewer Reject Signals

- The route moves code only to reduce file count.
- Hidden cross-file dependencies or scattered forward declarations replace the
  old public header.
- Edge-copy or publication ordering changes without explicit proof and intent.
- Prepared edge-copy or publication facts are replaced by local rescans.
- Expectations are weakened or supported edge-copy behavior is marked
  unsupported.
