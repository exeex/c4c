Status: Active
Source Idea Path: ideas/open/aarch64-codegen-01-calls-dispatch-bridge-helper-absorption.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Map Bridge Helpers and Durable Owners

# Current Packet

## Just Finished

Step 1 classified the exported `calls_dispatch_bridge.*` helpers and selected
one bounded helper family for the first absorption packet.

Classification:

- `lower_call_instruction`: public dispatch entry point. It is the top-level
  retained-call lowering handoff from `dispatch.cpp`; leave the public bridge
  entry in place until a broader call-instruction ownership change exists.
- `lower_scalar_call_argument_producers`: behavior-owning call-argument
  lowering. It recursively materializes scalar argument producers, local
  aggregate addresses, select chains, and scalar-state publications; not a
  first absorption target.
- `materialize_call_boundary_source_to_destination`: bridge-private adapter
  glue exposed for dispatch ordering. It converts a selected call-boundary
  frame-slot source into direct value publication when dispatch is about to
  emit a before-call move; candidate for a later materialization-owner packet.
- `materialize_indirect_call_callee_to_prepared_register`: behavior-owning
  indirect-callee lowering. It resolves local-load/store fallback and emits
  select-chain/csel materialization into the prepared callee register; leave
  in the bridge for now.
- `record_call_result_source_register`: bridge-private adapter glue. It records
  prepared call-result source registers into dispatch scalar state after the
  call; leave for a later result-publication packet.
- `call_boundary_move_reloads_materialized_address`: bridge-private adapter
  predicate exposed only because before-call move sequencing currently crosses
  the bridge boundary.
- `order_before_call_moves_for_source_preservation`: bridge-private adapter
  ordering exposed only because before-call move sequencing currently crosses
  the bridge boundary.
- `materialize_missing_frame_slot_call_arguments`: behavior-owning call-argument
  fallback materialization. It reconstructs missing frame-slot arguments from
  prepared homes and BIR values; leave until a dedicated argument-materialization
  owner packet.
- `publish_stack_preserved_call_values`: behavior-owning preservation
  publication. It chooses first stack-preserved values and emits stores to
  preservation homes; leave until a dedicated preservation-owner packet.

Chosen first family:

- Absorb the before-call move sequencing predicates:
  `call_boundary_move_reloads_materialized_address` and
  `order_before_call_moves_for_source_preservation`, with the private helper
  `move_source_aliases_destination`.
- Receiving owner: `src/backend/mir/aarch64/codegen/calls_moves.cpp`, declared
  through `calls.hpp` only if `dispatch.cpp` still needs to call the helpers.
  This file already owns `lower_before_call_moves`, `lower_after_call_moves`,
  and call-boundary move construction, so moving these pure
  `CallBoundaryMoveInstructionRecord` adapters there reduces bridge surface
  without changing ABI behavior.

## Suggested Next

Step 2 implementation suggestion: move
`move_source_aliases_destination`,
`call_boundary_move_reloads_materialized_address`, and
`order_before_call_moves_for_source_preservation` from
`calls_dispatch_bridge.cpp` to `calls_moves.cpp`; remove the two declarations
from `calls_dispatch_bridge.hpp`; add declarations to `calls.hpp` only for the
two helpers still called by `dispatch.cpp`; keep signatures and call sites
unchanged.

## Watchouts

- Keep the work behavior-preserving.
- Do not broaden into `dispatch.cpp`, whole-call-family cleanup, phase
  extraction, ABI redesign, or expectation rewrites.
- Leave helpers in the bridge when absorption would mix responsibilities or
  change call behavior.
- Do not absorb the recursive scalar argument, indirect callee, missing
  frame-slot argument, call-result recording, or stack-preservation helpers in
  the first code packet; each has a wider semantic/proof surface than the
  pure before-call move sequencing family.

## Proof

`git diff --check` passed. No build or `test_after.log` required by the
delegated proof for this classification-only packet.
