# AArch64 Scalar Cast ALU Publication Prepared Authority Repair

## Goal

Repair duplicate scalar publication authority across
`src/backend/mir/aarch64/codegen/cast_ops.cpp` and
`src/backend/mir/aarch64/codegen/alu.cpp` so pointer-derived cast and ALU
stack-publication paths consume prepared source/home facts without truncating
or reinterpreting live pointer registers as 32-bit scalar offsets.

## Why This Exists

The `00204` variadic aggregate route exposed a remaining mismatch after the
overflow cursor and edge-copy repairs: the join block keeps the selected
aggregate pointer live, but later scalar cast/ALU stack-publication lowering
emits a sequence like `mov w9, w13; sxtw x9, w9` for
`%t45.byte_offset.i64` / `%t45`. That sequence treats the current pointer
register as an older integer offset source and corrupts the address consumed by
aggregate byte-copy loads.

Existing open ideas do not cleanly own this combined surface. The dispatch
value-materialization idea owns `dispatch_value_materialization.cpp` and
explicitly excludes ALU/cast repairs. The ALU idea owns `alu.cpp` but not the
cast publication path. This idea is the smallest durable owner for the shared
scalar cast plus ALU publication gap.

## Owned Files

- `src/backend/mir/aarch64/codegen/cast_ops.cpp`
- `src/backend/mir/aarch64/codegen/alu.cpp`

Prepared lookup/query and placement files may be touched only when the scalar
cast and ALU publication paths need one shared source/home query or a prepared
source-preservation placement that does not already exist. Likely bounded
surfaces are:

- `src/backend/prealloc/prepared_lookups.cpp`
- `src/backend/prealloc/prepared_lookups.hpp`
- `src/backend/prealloc/publication_plans.cpp`
- `src/backend/prealloc/publication_plans.hpp`
- `src/backend/prealloc/regalloc/consumer_moves.cpp`
- `src/backend/prealloc/regalloc/consumer_moves.hpp`

## Current Dirty Context

The active worktree also contains incomplete, unaccepted changes in:

- `src/backend/mir/aarch64/codegen/memory.cpp`
- `src/backend/mir/aarch64/codegen/dispatch_edge_copies.cpp`

Those changes removed the original `00204` segfault and the bad cursor reload
before the join, but focused proof remains `7/8` with `00204`
`[RUNTIME_MISMATCH]`. This idea may use them as context during validation, but
must not claim them as complete progress unless a later lifecycle route
explicitly owns and proves them.

## Duplicated Helpers And Fallback Paths

- Cast lowering for scalar extension/truncation must not infer source width or
  source register identity from the currently live scratch register when
  prepared publication/source facts identify the real producer.
- ALU stack-publication paths for scalar derived values must not republish a
  stale register home when a prepared value-home, scalar-publication, or
  source-producer fact identifies the authoritative full-width source.
- Pointer-derived values such as `%t45.byte_offset.i64` and `%t45` must keep
  pointer and integer-offset authority separate; a selected pointer register
  cannot be reused as a 32-bit offset source merely because it occupies the
  expected scratch register.
- Any shared helper between cast and ALU lowering should answer source/home
  selection, not become another same-block BIR scan.

## Shared Facts To Consume Or Add

- Consume `PreparedValueHome`, `PreparedStoragePlanValue`,
  `PreparedScalarPublicationPlan`, and source-producer lookups where they
  already identify the scalar source or destination home.
- Consume prepared block-entry or edge-publication facts when a join block
  already has an authoritative incoming pointer value.
- Add a small prepared scalar cast/publication source query only if existing
  prepared facts cannot answer the source for both `cast_ops.cpp` and
  `alu.cpp` without raw value-name or same-block producer scans.
- Preserve or place source values before block-entry or edge publication
  supersedes their physical register when later scalar cast/ALU publication
  still needs the original value and reloading memory would observe mutated
  state.
- Reuse the dispatch/select-chain query only if tracing proves the cast/ALU
  publication path needs an already-prepared selected pointer source; do not
  move the repair back into `dispatch_value_materialization.cpp` without that
  proof.

## Out Of Scope

- Do not change AAPCS64 call staging, variadic layout constants, edge-copy
  mechanics, or memory cursor writeback behavior under this idea.
- Do not extend `dispatch_value_materialization.cpp` unless a bounded audit
  proves a shared prepared query must be exposed for cast/ALU consumers.
- Do not change arithmetic opcode spelling, target-local scratch ordering, or
  sign/zero-extension semantics except to consume the correct prepared source.
- Do not accept the dirty `memory.cpp` or `dispatch_edge_copies.cpp` changes as
  part of this idea unless the supervisor explicitly routes them into a
  coherent combined slice with proof.

## Acceptance Criteria

- The first bad `00204` scalar publication site is traced to a concrete cast or
  ALU lowering path with the prepared source/home fact that should control it.
- If the stale source has no safe current home, prepared planning preserves or
  relocates that source before block-entry/edge publication overwrites its
  physical register, rather than making cast lowering reload a mutated local.
- Cast and ALU publication lowering consume prepared source/home authority for
  pointer-derived scalar publications instead of reusing stale live registers
  or raw BIR producer scans.
- The `mov w9, w13; sxtw x9, w9` pointer-corrupting sequence is absent for the
  `vaarg.join.39` aggregate byte-copy path for semantic reasons, not by
  testcase-shaped filtering.
- The focused eight-test subset passes or, if another owner is exposed, the
  route records a precise split before more implementation.
- Same-feature probes remain active and no expectation is downgraded.

## Reviewer Reject Signals

- Reject named-case fixes for `00204`, `myprintf`, `%t45`, `%t49`, or
  `vaarg.join.39` instead of a general cast/ALU prepared-source rule.
- Reject new raw BIR scans, value-name matching, or same-block producer walks
  used as the durable source of truth when prepared facts exist.
- Reject changes that only suppress `mov w9, w13; sxtw x9, w9` textually
  without proving the selected pointer and integer offset sources are separated.
- Reject cast-only changes that detect stale register authority but then reload
  mutable memory at the join instead of preserving the original source value
  before block-entry or edge publication overwrites its register.
- Reject expectation downgrades, unsupported-test rewrites, helper renames, or
  classification-only notes claimed as capability progress.
- Reject committing the dirty memory or edge-copy context under this idea
  unless the route explicitly owns the combined slice and proves the full
  focused subset.
