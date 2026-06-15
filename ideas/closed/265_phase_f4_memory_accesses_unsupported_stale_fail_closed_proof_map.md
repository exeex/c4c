# 265 Phase F4 memory_accesses unsupported/stale fail-closed proof map

## Goal

Map the remaining unsupported and stale fail-closed proof obligations for
`PreparedFunctionLookups::memory_accesses` after the Phase F3 Route 3 x86
`LoadLocal` agreement bridge work.

This is analysis/proof work only. It must not demote, delete, privatize, or
wrap the memory lookup table.

## Why This Exists

The post-F3 gate found `memory_accesses` thinner than after idea 247 because
x86 now has positive selected `LoadLocal` source-memory agreement evidence and
supporting fixture/compare-join coverage. The row is still blocked because the
unsupported and stale prepared-state surfaces do not yet have a bounded
fail-closed proof map.

## In Scope

- Identify the current semantic authority for Route 3 memory/source facts.
- Map prepared-only memory access rows.
- Map stale-publication rows.
- Map byte-offset drift rows.
- Map cross-publication mismatch rows.
- Name the observable fail-closed behavior expected for each row.
- Name the x86 and riscv evidence, or explicit non-applicability, needed before
  any later demotion proposal can be considered.

## Out Of Scope

- Memory lookup demotion, deletion, privatization, accessor wrapping, or adapter
  migration.
- Broad `PreparedFunctionLookups` or prepared aggregate retirement.
- Moving target storage, addressing, source-home, helper, ABI, layout, register,
  stack, wrapper, formatting, emission, or exact target-output policy into BIR.
- Rewriting tests, helpers, or diagnostics to make unsupported/stale rows look
  accepted.

## Completion Criteria

- A proof map exists for prepared-only, stale-publication, byte-offset drift,
  and cross-publication mismatch behavior.
- Each row names the semantic authority, public prepared compatibility surface,
  consumer boundary, and fail-closed expectation.
- The map separates supported proof from missing proof and does not claim
  demotion readiness.
- Any future implementation candidate is blocked unless the map names a real
  consumer path plus stable helper/oracle/status/fallback/route-debug/printer,
  wrapper-output, target-output, and baseline behavior.

## Reviewer Reject Signals

- The slice weakens unsupported expectations, helper/oracle statuses, fallback
  names, route-debug output, prepared-printer output, wrapper output, exact
  target output, or baseline behavior.
- The slice claims capability progress through expectation rewrites, helper
  renames, oracle/status relabeling, route-debug/printer formatting changes, or
  classification-only table edits.
- The slice hides old public `memory_accesses` authority behind a renamed BIR
  field, route field, private accessor, adapter, wrapper, or compatibility
  helper without proving it is only a mirror.
- The slice migrates target-owned storage, addressing, source-home, helper,
  ABI, layout, register, stack, formatting, emission, instruction spelling, or
  wrapper policy into BIR.
- The slice adds named-testcase shortcuts or proves only a single fixture while
  nearby prepared-only, stale, offset-drift, or cross-publication mismatch rows
  remain unexamined.
- The slice authorizes broad prepared retirement, `PreparedFunctionLookups`
  retirement, or memory lookup demotion.

## Closure Note

Status: Closed as an analysis-only proof-map result.

Close-time regression guard used the existing matching full-suite logs
`test_before.log` and `test_after.log`; both recorded `3428/3428` passing
tests, and the monotonic guard passed with `--allow-non-decreasing-passed`.

The final bounded proof map covers four families:

- Prepared-only rows: PO-3 has partial x86 selected-path guardrail evidence
  when a selected `LoadLocal` agreement candidate exists. PO-4 has partial riscv
  consumer proof for missing shared lookups, missing publication, missing
  prepared memory access, incomplete source memory, and non-`LoadLocal`
  source-memory unavailability. PO-1 and PO-2 have helper-level lookup and
  source-memory publication proof. PO-5 remains fallback/helper-reader state,
  not semantic authority. All PO rows remain blocked for demotion because no
  proof shows every still-public `memory_accesses` consumer rejects a
  self-consistent prepared-only row at the same Route 3/Route 5 authority
  boundary while preserving helper/status/fallback/debug/printer/wrapper/output
  behavior.
- Stale-publication rows: SP-1 and SP-2 have selected x86 proof for reachable
  Route 5/Route 3 disagreement, missing source-memory, source-producer index
  mismatch, and incomplete publication rows. SP-3 has riscv consumer proof for
  dynamic memory-source byte-offset and incomplete Route 3 mutations preserving
  output while setting agreement booleans false. SP-4 has helper/oracle proof,
  and SP-5 remains fallback/helper state that must be joined to current Route
  3/Route 5 authority before semantic use. All SP rows remain blocked because
  no synthetic stale-row matrix proves old prepared facts fail closed for
  producer block, instruction index, source value, base kind, wrong edge,
  duplicate, and obsolete Route 5 owner across both x86 and riscv same-consumer
  surfaces.
- Byte-offset drift rows: BO-1 has selected x86 guardrail proof for a reachable
  selected `LoadLocal` offset mutation and source checks for effective Route 3
  offset equality before agreement. BO-2 has riscv consumer proof that an
  embedded Route 3 offset mutation preserves prepared output while agreement
  booleans become false. BO-3 and BO-4 have helper-level offset mismatch proof,
  and BO-5 has partial x86/riscv guardrail evidence. All BO rows remain blocked
  because current proof is selected-path or helper-level and does not cover
  every synthetic prepared offset drift row, public lookup consumer, target
  operand surface, or wrapper/exact-output contract.
- Cross-publication mismatch rows: CP-1 has partial x86 selected-path proof for
  Route 5/Route 3 source-memory mismatch and producer-index mismatch, plus
  riscv diagnostic-path proof for scalar Route 5 no-match and dynamic
  memory-source mismatch/incomplete rows. CP-5 has partial cross-target evidence
  that x86 rejection and riscv preserved output can both be valid when
  agreement diagnostics fail closed. CP-2, CP-3, and CP-4 have helper/MIR/Route
  5 proof for missing source/destination, wrong predecessor/successor,
  destination type mismatch, duplicate Route 5 diagnostics, non-memory producer
  handling, local source-memory mismatch, and explicit no-source/no-match
  statuses. All CP rows remain blocked because no combined memory-source target
  proof exhausts internally consistent but wrong prepared owners,
  duplicate/conflicting Route 5 memory-source rows, wrong-edge/wrong-destination
  memory-source rows, and cross-target status/fallback/output matrices.

Explicitly non-applicable rows remain outside the demotion criteria:
target-owned local-slot, stack, register, addressing, wrapper, formatting,
exact-output policy, large-offset syntax, base-plus-offset legality, frame-slot
layout, stack offset choice, target operand spelling, and target differences
between x86 selected-output rejection and riscv preserved prepared-backed
output.

Disposition: `PreparedFunctionLookups::memory_accesses` remains public
compatibility. The proof map separates selected-path/helper-level guardrails
from missing same-consumer fail-closed proof and does not authorize demotion,
private-accessor work, implementation work, expectation weakening, or public
lookup retirement. No separate follow-up idea was created from this map.
