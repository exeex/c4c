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
