# RV64 Move-Bundle Target Materialization

Status: Closed
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

## Closure Note

Closed on 2026-07-08 after RV64 object emission consumed explicit prepared
source and destination homes for authorized
`block_entry/out_of_ssa_parallel_copy/phi_join_register_to_register`
move-bundle rows. Focused movement included `src/20000314-1.c`,
`src/20040309-1.c`, `src/pr63641.c`, and the representative
`src/20020206-2.c` compiling through RV64 object emission, with the Step 3
probe summary recording 41 compile-through rows.

Remaining `src/pr71631.c` repeated stack-destination behavior is not counted
as supported by this idea. It requires explicit destination fan-in/order
authority and is tracked separately in
`ideas/open/622_repeated_stack_destination_fan_in_order_authority.md`.
Rows that only rerouted to downstream owners were route-separation evidence,
not target-support proof.

## Reviewer Reject Signals

- Reject RV64 guessing of move sources or destinations.
- Reject named-case-only materialization for `src/20020206-2.c` or similar.
- Reject folding destination fan-in design into this implementation.
- Reject expectation, unsupported-marker, allowlist, timeout, runtime, or
  accounting changes.
- Reject retaining the unsupported move-bundle target shape behind a renamed
  helper.
