# 250 Phase F3 Route 3 memory source parity blocker map

## Idea Type

x86/riscv parity proof.

## Goal

Prove or explicitly block the narrow Route 3 memory/source identity boundary
needed before public `memory_accesses` lookup helpers can become compatibility
mirrors instead of prepared semantic authority.

## Why This Exists

Phase F2 found riscv Route 3/source-memory evidence but missing or indirect x86
evidence. Public prepared memory lookup and source-memory statuses remain
observable authority until x86 and riscv either consume the same route/BIR
memory/source fact or one target is explicitly excluded with fail-closed proof.

## In Scope

- Map one memory/source identity fact from route/BIR publication to prepared
  memory lookup agreement.
- Identify the x86 memory/source consumer gap or a concrete x86 consumer to
  prove.
- Recheck the riscv Route 3/status rows against the same semantic fact.
- Require fail-closed behavior for missing, invalid, duplicate/conflict,
  mismatch, unsupported, prepared-only, fallback, and policy-sensitive cases.
- Preserve prepared source-memory statuses, helper/oracle names, fallback
  names, unsupported rows, and addressing/storage-sensitive output.

## Out Of Scope

- Deleting or privatizing `PreparedFunctionLookups::memory_accesses`.
- Moving addressing modes, frame/storage placement, load/store instruction
  choice, register materialization, or target output formatting into BIR.
- Rewriting memory baselines, prepared status strings, or helper/oracle names.
- Broad memory lowering cleanup outside the selected memory/source identity.

## Acceptance Criteria

- The packet proves one shared route/BIR memory/source identity for named x86
  and riscv evidence, or records exactly why the boundary is still blocked.
- Prepared lookup/status answers agree with the route/BIR fact or fail closed
  on mismatch.
- Target addressing and storage policy remain target-owned.
- The result states whether a later one-adapter implementation idea is safe.

## Reviewer Reject Signals

- Reject named-case shortcuts that only match a single memory testcase or one
  riscv status row while claiming cross-target memory-source parity.
- Reject unsupported expectation downgrades, source-memory status weakening,
  or helper/oracle contract weakening without explicit approval.
- Reject helper renames, expectation rewrites, or classification-only matrix
  updates claimed as memory/source capability progress.
- Reject broad load/store, frame layout, addressing, storage, or register
  materialization rewrites outside the selected identity fact.
- Reject any implementation that keeps prepared `memory_accesses` as the old
  semantic source behind a new route/BIR accessor name.
