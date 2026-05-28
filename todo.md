Status: Active
Source Idea Path: ideas/open/66_aarch64_local_dispatch_block_route.md
Source Plan Path: plan.md
Current Step ID: Step 1
Current Step Title: Inventory Local Dispatch Route Responsibilities

# Current Packet

## Just Finished

Step 1 completed: inventoried the AArch64 local dispatch route without
implementation edits.

Public block entry points and call sites:
- `make_block_lowering_context` is public in `dispatch.hpp`, used by
  production traversal in `traversal.cpp`, and directly by AArch64/backend
  tests that construct block contexts for diagnostics, return lowering, branch
  lowering, memory operand, call-boundary, and instruction-dispatch coverage.
- `dispatch_prepared_block` is public in `dispatch.hpp`, used by production
  traversal in `traversal.cpp`, and directly by the same focused tests to
  validate block traversal, unsupported diagnostics, before-return publication,
  branch fusion, memory retry, and instruction ordering.
- Both public entries are route-local and still justified; no current public
  entry point can be removed without changing production traversal or broad
  direct-test coverage.

Route-local responsibilities:
- `classify_instruction`, `append_block_diagnostic`,
  `unsupported_terminator_message`, and
  `append_unsupported_instruction_diagnostic` are route-local diagnostic
  helpers. They keep AArch64-specific missing-context, missing-BIR mapping,
  unsupported-instruction, and unsupported-terminator diagnostics durable.
- `find_bir_block` and `make_block_lowering_context` are route-local
  BIR/prepared block mapping and context construction. They should remain local
  to the AArch64 block route, not shared.
- `before_return_publication_already_emitted` and
  `record_before_return_publication` are route-local before-return publication
  sequencing helpers that preserve final return move ordering.
- `make_dispatch_branch_fusion_hooks` is route-local hook wiring. It passes
  owner functions into comparison lowering but keeps branch-fusion sequencing
  visible in `dispatch.cpp`.
- The main `dispatch_prepared_block` loop is route-local final ordering:
  entry formal publications, block-entry moves, per-instruction lowering,
  address/materialization retry routing, atomics, before-return moves, and
  terminator emission are appended in target-specific order.

Precise-owner candidates or private cleanup:
- `lower_store_local_with_address_materialization` and
  `lower_scalar_with_address_materialization` are route-local retry adapters
  over memory/address/scalar owners. Keep them local unless a future packet can
  move only implementation details without hiding retry ordering.
- `instruction_result_value` and `instruction_result_has_stack_home` are
  target-route filters for prepared-query suppression. Keep local for now.
- `align_to`, `binary_uses_named_value`, `is_store_local_instruction`,
  `NarrowLocalStorePublication`, `block_context_for_label`,
  `EdgeProducerContext`, and the private `make_bir_machine_instruction` in
  `dispatch.cpp` have no call sites in that file. They are the clearest first
  cleanup/narrowing candidates.
- The duplicate `make_bir_machine_instruction` helpers in other owner files
  are out of scope for this plan unless a separate owner introduces a shared
  target-local utility; Step 2 should not broaden into that.

Out-of-scope semantic authority:
- Producer discovery, publication planning, value materialization recursion,
  prepared join-query authority, and target emission spelling are delegated to
  precise owners. `dispatch.cpp` should continue to call those owners without
  rebuilding their facts locally.
- Branch-fusion sequencing, before-return publication ordering,
  prepared-memory index retry decisions, and final instruction append order
  must stay visible in the local route.

## Suggested Next

Step 2 should preserve both public block entry points and narrow private route
surface instead: remove the unused private helpers/scaffolding in
`dispatch.cpp` (`align_to`, `binary_uses_named_value`,
`is_store_local_instruction`, `NarrowLocalStorePublication`,
`block_context_for_label`, `EdgeProducerContext`, and the private
`make_bir_machine_instruction`) after confirming no hidden call sites. Do not
change traversal, diagnostics, sequencing, or public signatures.

## Watchouts

This plan preserves the local AArch64 block route. Do not move block traversal
or target diagnostics to shared code, do not reintroduce producer/publication
or value-materialization authority into `dispatch.cpp`, and do not bundle idea
67's local-slot offset probe into this route.

Proof coverage needed after code changes: production traversal through
`traversal.cpp`, direct `dispatch_prepared_block` tests, unsupported
instruction and unsupported terminator diagnostics, missing block/BIR mapping
diagnostics, before-return publication, branch-fusion hooks, prepared-memory
retry routing, current-block join suppression, and representative prepared
block lowering.

## Proof

Read-only inventory plus `git diff --check`. Inspection commands included
targeted reads of `plan.md`, idea 66, `dispatch.hpp`, and `dispatch.cpp`, plus
`rg` scans for public entry-point call sites, diagnostics/branch-fusion proof
coverage, private helper call sites, and owner-local `make_bir_machine_instruction`
duplicates.
