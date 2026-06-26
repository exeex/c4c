# Prepared Global-Symbol Memory Access Publication

Status: Closed
Type: Upstream prepared-data contract follow-up
Parent: `ideas/open/354_rv64_gcc_torture_prepared_module_shape_classification.md`
Split From: `ideas/closed/383_rv64_global_aggregate_lane_materialization.md`
Related: `ideas/closed/357_rv64_object_route_data_sections_globals_strings.md`

## Goal

Publish authoritative prepared global-symbol memory-access facts for BIR loads
from global addresses, including aggregate lane loads currently represented as
`LoadLocalInst` with `bir::MemoryAddress::BaseKind::GlobalSymbol`.

## Why This Exists

Idea 383 proved RV64 object emission can consume explicit prepared
`GlobalSymbol` memory-access facts and must fail closed when the only available
shape is raw global-address spelling. The representative
`src/20030914-2.c` now stops at:

```text
unsupported_global_data: RV64 object route requires prepared global-symbol memory access facts for LoadLocalInst global-address lanes
```

The first failing instruction is:

```text
%t0.global.aggregate.load.0 = bir.load_local i32 %t0.0, addr gs
```

The target route has no authoritative prepared record that names the global
symbol base, lane offset, access width, section/storage facts, initializer or
lane payload relationship, address space, alignment, and relocation/address-use
semantics for this load. That information belongs in the prepared-data
producer/contract before target object emission consumes it.

## In Scope

- Audit the producer path that emits `LoadLocalInst` global-address lanes for
  `src/20030914-2.c`.
- Define the prepared memory-access contract for global-symbol load lanes,
  including symbol identity, offset, access size, alignment, address space, and
  relocation/address-use meaning.
- Publish global-symbol memory-access facts for supportable static global data
  loads without relying on source names or testcase identity.
- Add prepared-printer or contract tests that show the facts are present before
  target RV64 object emission consumes them.
- Rerun `src/20030914-2.c` and document whether the representative advances to
  aggregate `va_arg`, terminator, call, control-flow, or another owner.

## Out of Scope

- Reopening RV64 target-side inference from raw `LoadLocalInst addr <global>`
  spelling; idea 383 closed that as fail-closed target behavior.
- Reopening ELF data sections, symbols, relocations, strings, or globals already
  published by idea 357.
- Aggregate `va_arg` helper lowering; use
  `ideas/open/371_rv64_object_route_aggregate_va_arg_helper_lowering.md`.
- Non-register parameter homes; use
  `ideas/open/374_rv64_object_route_non_register_param_homes.md`.
- Inferring aggregate initializer bytes, symbol identity, or layout from C
  source syntax, testcase names, raw log text, or target diagnostics.

## Acceptance Criteria

- Focused prepared-contract tests prove global-symbol memory-access facts are
  published for the first supportable aggregate lane load shape.
- Existing RV64 object-emission fixtures for explicit prepared `GlobalSymbol`
  facts remain green without target-side raw-address inference.
- `src/20030914-2.c` advances beyond the current
  `unsupported_global_data` boundary, or records a more precise upstream
  unsupported publication reason.
- Any remaining global data shapes are documented as separate owners or
  fail-closed unsupported facts, not silently inferred by target emission.

## Reviewer Reject Signals

- Reject named-case-only handling for `src/20030914-2.c` or global `gs`.
- Reject target-side lowering that reconstructs global-symbol facts from raw
  `LoadLocalInst addr <global>` spelling instead of consuming prepared facts.
- Reject publishing incomplete facts that omit symbol identity, offset, access
  size, address space, alignment, or relocation/address-use meaning while
  claiming the target can lower the lane semantically.
- Reject expectation downgrades, allowlist filtering, unsupported-contract
  weakening, or diagnostic-only renames claimed as prepared publication
  progress.
- Reject reopening broad data-section/global/string scope from idea 357 unless
  the diff proves a missing prepared-publication contract shared by this lane
  shape.
- Reject retaining the exact `unsupported_global_data` failure for the
  representative behind renamed helpers or moved diagnostics.

## Closure Notes

Closed after the producer path for aggregate global load lanes was repaired to
publish canonical `bir.load_global` instructions and matching prepared
`base=global_symbol` memory-access records. The representative
`src/20030914-2.c` now publishes global-symbol lane facts through offset `68`,
including symbol identity, offset, access size, alignment, and
base-plus-offset support, and RV64 object emission consumes those explicit
facts without reintroducing raw `LoadLocalInst addr <global>` inference.

The focused prepared-contract and RV64 object-emission tests passed after the
producer change. The representative advanced beyond the old
`unsupported_global_data` boundary and now stops at a distinct same-module call
lowering boundary:

```text
%t1 = bir.call i32 f(ptr byval(size=72, align=4) %t0, i32 4660)
unsupported_instruction_fragment: BIR instruction requires unsupported RV64 object lowering
```

That next boundary is not a prepared global-symbol publication gap. It is
handed off to
`ideas/open/386_rv64_object_route_same_module_byval_aggregate_call_args.md`.

Close-time regression guard used matching focused backend logs:

- `test_before.log`: passed=2 failed=0 total=2
- `test_after.log`: passed=2 failed=0 total=2
- no newly failing tests
