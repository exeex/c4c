# AArch64 Shared Same-Block Materialization Authority

Ownership class: shared-authority migration

## Goal

Move same-block scalar/source-producer identity decisions out of AArch64
dispatch code and into shared prepared/query authority, while keeping target
recursive emission local after source facts are known.

## Why This Exists

Idea 59 found remaining same-block materialization paths where AArch64 dispatch
can still discover semantic producer identity through local recursive scans.
That duplicates the prepared/shared authority model repaired by ideas 47
through 49 and makes nearby scalar, load-local, integer-constant, ALU,
comparison, and FP paths depend on target-local semantic recovery.

## In Scope

- `src/backend/mir/aarch64/codegen/dispatch_lookup.cpp` same-block scalar and
  load-local availability helpers.
- `src/backend/mir/aarch64/codegen/dispatch_value_materialization.cpp`
  recursive same-block/source discovery.
- Relevant `src/backend/mir/aarch64/codegen/dispatch_producers.*` wrapper
  consumers.
- The shared prepared/query owner that should expose the replacement same-block
  source facts.

## Out Of Scope

- Moving AArch64 register allocation, scratch handling, instruction spelling,
  or recursive emission sequencing to shared code.
- Solving edge-copy dependency fallback, select-chain direct-global discovery,
  or join-copy query migration.
- Adding testcase-shaped AArch64 shortcuts for one named same-block case.
- Rewriting unrelated dispatch-family layout.

## Acceptance Criteria

- AArch64 materialization asks shared/prepared authority for same-block source
  facts instead of deciding semantic producer identity through local scans.
- Local AArch64 recursion remains limited to target emission from already-owned
  source facts.
- Missing prepared/shared facts fail closed or produce a durable diagnostic
  instead of silently falling back to target-local semantic discovery.
- Focused proof covers scalar materialization, load-local reuse, integer
  constants, ALU/comparison/FP recursive value publication, and nearby negative
  cases for missing prepared facts.

## Reviewer Reject Signals

- A new named-case or same-block scan is added under AArch64 dispatch.
- Recursive materialization still decides semantic producer identity locally
  after the migration.
- Proof covers only one narrow testcase while adjacent scalar or load-local
  paths remain unaudited.
- Expectations are weakened or supported paths are marked unsupported to hide
  missing shared facts.
- The old local failure mode survives behind a new shared-looking helper name.
