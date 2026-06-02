# AArch64 Memory Owner Subresponsibility Audit

## Goal

Audit `src/backend/mir/aarch64/codegen/memory.cpp` after dispatch publication,
edge-copy, local-helper, and prepared-wrapper contraction, then produce scoped
implementation ideas only for memory subresponsibilities whose owner boundary is
clear.

## Why This Exists

Memory became the clearest post-contraction residue. Ideas 80 and 81 moved
owner-local memory helpers back from dispatch publication and edge-copy files,
and idea 84 removed only the redundant prepared wrapper surfaces. The result is
a more correct but larger memory owner.

This route should decide whether the growth is justified target-local memory
lowering, or whether memory now contains separable subresponsibilities such as
frame-slot address materialization, stack-source publication, store
retargeting, identity validation, or prepared memory-record construction.

## Owned Files

- `src/backend/mir/aarch64/codegen/memory.cpp`
- `src/backend/mir/aarch64/codegen/memory.hpp`
- Read-only comparison against prior closed ideas:
  `ideas/closed/70_aarch64_memory_prepared_address_authority_cleanup.md`,
  `ideas/closed/80_aarch64_dispatch_publication_owner_relocation.md`,
  `ideas/closed/81_aarch64_dispatch_edge_copy_owner_contraction.md`,
  `ideas/closed/83_aarch64_local_helper_duplication_tail_cleanup.md`, and
  `ideas/closed/84_aarch64_prepared_consumer_wrapper_contraction.md`.

## In Scope

- Build a function-level inventory of memory owner responsibilities.
- Classify helper clusters as:
  - `target-local-memory-emission`
  - `frame-slot-addressing`
  - `stack-source-publication`
  - `store-retargeting`
  - `identity-validation`
  - `prepared-record-construction`
  - `diagnostics-and-error-spelling`
  - `candidate-local-subowner`
  - `needs-shared-authority-evidence`
- Produce follow-up ideas only when a cluster has a stable owner boundary and
  proof route.
- Explain which large clusters should stay in `memory.cpp`.

## Out Of Scope

- Direct implementation edits in `memory.cpp`.
- Moving AArch64 addressing legality, scratch choice, register spelling, or
  machine-record construction into shared BIR/prealloc.
- Reopening dispatch publication or edge-copy relocation.
- Treating line-count reduction as success without preserving memory lowering
  ownership.

## Proof Expectations

- This is audit-only; no backend tests are required unless implementation files
  are accidentally touched.
- The audit close note must name any follow-up ideas created and explicitly say
  which memory clusters remain intentionally target-local.

## Reviewer Reject Signals

- The audit proposes one monolithic "shrink memory.cpp" implementation route.
- It says "move to BIR" without naming a missing target-neutral fact.
- It reopens dispatch relocation despite 80/81 closure notes.
- It treats owner relocation growth as a regression without evidence.

## Close Note

Closed after the active audit runbook reached Step 5 and `todo.md` recorded a
close-ready audit summary.

The audit found that the following memory clusters remain intentionally
target-local in `memory.cpp`: load/store instruction emission, scalar memory
mnemonic and opcode selection, memory operand support decisions, frame-slot
base and offset policy, stack-source publication emission, store retargeting
around prepared memory records, identity validation against selected memory
operands, prepared memory operand/instruction record construction, diagnostic
spelling, and variadic `va_list` field memory handling. These clusters depend
on AArch64 addressing legality, `sp`/`x29` base policy, scratch selection,
register spelling, ABI-sensitive result handling, machine-record construction,
or prepared-memory fact validation, so the audit did not justify moving them
to shared BIR/prealloc authority.

The audit created two narrow follow-up implementation ideas:

- `ideas/open/88_aarch64_memory_frame_slot_address_materialization_owner.md`
  for a local AArch64 memory frame-slot/address materialization owner.
- `ideas/open/89_aarch64_memory_store_retargeting_owner.md` for a local
  AArch64 memory store-retargeting owner.

The audit rejected a monolithic `memory.cpp` shrink route, shared-authority
identity-validation relocation without a named target-neutral fact, reopening
dispatch publication or edge-copy relocation from ideas 80 and 81, and
prepared-wrapper resurrection that would undo idea 84.

No implementation files were changed for this audit. Close-time regression
guard used a fresh same-scope backend-name comparison:

```bash
cmake --preset default
cmake --build --preset default
ctest --test-dir build -j --output-on-failure -R '^backend_' > test_before.log
cmake --preset default
cmake --build --preset default
ctest --test-dir build -j --output-on-failure -R '^backend_' > test_after.log
python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py \
  --before test_before.log \
  --after test_after.log \
  --allow-non-decreasing-passed
```
