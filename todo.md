Status: Active
Source Idea Path: ideas/open/167_route1_producer_constant_oracle_thinning.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Validate And Handoff

# Current Packet

## Just Finished

Completed Step 5: validated and handed off the Route 1 producer/constant oracle
thinning slice.

Selected migration completed by this runbook:
- `src/backend/mir/aarch64/codegen/dispatch_value_materialization.cpp` now uses
  `mir::find_same_block_scalar_producer(...)` and
  `mir::evaluate_same_block_integer_constant(...)` for the selected AArch64 GP
  scalar value-publication same-block producer and named integer-constant facts.
- Immediate fast paths, prepared home/memory/publication lookups,
  select-chain materialization, and target emission policy were left intact.
- The local prepared-shaped adapter in the selected file is residual
  compatibility for existing AArch64 target-policy APIs, not prepared producer
  discovery.

Step 4 handoff result: no prepared producer, constant, cache, or API surface is
safe to contract yet. Remaining direct consumers still require the prepared
surfaces outside the migrated GP scalar value-publication path.

Residual consumers blocking contraction:
- `dispatch_value_materialization.cpp` still uses prepared select-chain and
  publication surfaces for compatibility.
- `src/backend/mir/aarch64/codegen/dispatch_producers.cpp` still uses prepared
  publication-source producer queries.
- `src/backend/mir/aarch64/codegen/fp_value_materialization.cpp` still uses
  prepared same-block scalar producer lookup for FP materialization.
- `src/backend/mir/aarch64/codegen/alu.cpp` still uses prepared same-block
  load-local, scalar producer, and select-chain materialization helpers.
- `src/backend/mir/aarch64/codegen/calls.cpp` still uses prepared
  source-producer lookups for scalar call arguments and indirect callees.
- `src/backend/prealloc/comparison.cpp`,
  `src/backend/prealloc/call_plans.cpp`,
  `src/backend/prealloc/publication_plans.cpp`, and
  `src/backend/prealloc/select_chain_lookups.cpp` still expose prepared helper
  APIs used by non-selected paths.

## Suggested Next

The active runbook is ready for supervisor acceptance and plan-owner
close/deactivation review as a completed no-contraction slice. No additional
implementation packet is recommended from this runbook without a new selected
consumer family.

## Watchouts

- `dispatch_value_materialization.cpp` no longer calls the prepared same-block
  producer or prepared integer-constant helpers directly for the migrated
  semantic facts, but it still depends on prepared select-chain/publication
  surfaces.
- Do not move homes, registers, storage, emitted-register state, operand views,
  frame slots, final instruction order, spill/reload behavior, or publication
  hooks into BIR.
- Reject testcase-shaped shortcuts, expectation rewrites, or narrow named-case
  matching as progress.
- Residual prepared consumers are blockers for contraction, not cleanup targets
  for this plan step.
- The local prepared-shaped adapter in the selected file reshapes a MIR
  producer answer for existing AArch64 target-policy APIs; it does not by
  itself justify hiding prepared lookup APIs.
- Treat the residual prepared consumers as evidence that this source idea
  remains broader than the completed selected slice; close/deactivate choice
  belongs to the plan owner.

## Proof

Supervisor-selected proof command run:
`bash -lc "set -o pipefail; cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_prepared_lookup_helper|backend_aarch64_instruction_dispatch)$'" |& tee test_after.log`

Result: passed. The build was already up to date, and both selected tests
passed:
- `backend_prepared_lookup_helper`
- `backend_aarch64_instruction_dispatch`

Proof log: `test_after.log`.
