# RV64 Move-Bundle Target Materialization

Status: Open
Type: Implementation
Parent: `ideas/open/601_rv64_gcc_torture_1000_pass_recovery_umbrella.md`
Related:
- `ideas/closed/587_prepared_value_freshness_authority_mvp.md`
- `ideas/closed/588_shared_prealloc_move_operand_source_freshness_inventory.md`
- `docs/rv64_gcc_torture_1000_pass_recovery/failure_bucket_map.md`
Owning Layer: RV64/MIR consumer
Queue Order: 9
Prerequisites: upstream prepared source and destination authority must be present; destination fan-in rows from `607` remain blocked until researched
Estimated Evidence Breadth: `75` out-of-SSA move-bundle target-shape rows
Proof Surface: prepared move-bundle rows whose authority exists but RV64 lacks target materialization

## Goal

Implement RV64 consumer lowering for supported out-of-SSA move-bundle target
shapes where prepared authority already proves the move operands and
destinations.

## Why This Exists

The largest RV64/MIR consumer sub-bucket is `75` out-of-SSA move-bundle target
shape rows. These are target-consumer failures only when upstream authority is
already available.

## In Scope

- RV64 materialization of supported move-bundle target shapes.
- Guardrails that reject rows with missing prepared authority or unresolved
  destination fan-in.
- Same-family proof across many move-bundle rows.

## Out Of Scope

- Prepared/prealloc authority production.
- Destination fan-in policy.
- Terminator lowering, general instruction fragments, ABI calls/returns,
  runtime, expectations, unsupported markers, allowlists, timeouts, or
  accounting.

## Acceptance Criteria

- Multiple move-bundle target-shape rows progress through RV64 lowering.
- Rows lacking source, stack slot, branch operand, or destination authority
  remain rejected with accurate diagnostics.
- The proof demonstrates target consumption rather than producer repair.

## Reviewer Reject Signals

- Reject RV64 guessing of move sources or destinations.
- Reject named-case-only materialization for `src/20020206-2.c` or similar.
- Reject folding destination fan-in design into this implementation.
- Reject expectation, unsupported-marker, allowlist, timeout, runtime, or
  accounting changes.
- Reject retaining the unsupported move-bundle target shape behind a renamed
  helper.
