Status: Active
Source Idea Path: ideas/open/aarch64-codegen-01-calls-dispatch-bridge-helper-absorption.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Repeat Only for Nearby Same-Shape Helpers

# Current Packet

## Just Finished

Step 3 audited the remaining `calls_dispatch_bridge.cpp/.hpp` helpers after
the Step 2 before-call move sequencing absorption.

Decision: stop; no remaining helper family is same-shape enough for another
bounded absorption packet under the Step 3 rule.

Remaining exported bridge helpers and reasons to leave them in place:

- `lower_scalar_call_argument_producers` drives recursive scalar argument
  materialization, direct global select-chain argument publication, local
  aggregate address fallback, and scalar-state publication.
- `lower_call_instruction` remains a public dispatch bridge entry point for
  dynamic-stack helper calls, variadic helper handling, prepared call-plan
  diagnostics, and final prepared call lowering.
- `materialize_call_boundary_source_to_destination` is the nearest-looking
  before-call helper, but it materializes a missing frame-slot source value
  into a call ABI destination and records scalar-state publication; that is a
  wider semantic surface than Step 2's pure before-call move ordering/reload
  skip helpers.
- `materialize_indirect_call_callee_to_prepared_register` owns indirect callee
  lowering, including local-load/store resolution, select-chain/csel emission,
  scratch selection, and call ABI publication.
- `record_call_result_source_register` records after-call GPR/FPR result source
  registers from prepared move and result facts.
- `materialize_missing_frame_slot_call_arguments` materializes missing
  frame-slot call arguments into ABI registers and records scalar-state
  publication.
- `publish_stack_preserved_call_values` publishes stack-preserved values using
  prepared call-plan lookup/fallback behavior and stack-slot stores.

## Suggested Next

Proceed to Step 4 focused behavior-preservation proof for the already-completed
Step 2 absorption slice. Suggested coverage: build proof plus focused AArch64
MIR/codegen tests covering normal calls, select-chain call arguments,
call-result source registers, preserved-value materialization, and local-load
fallback call arguments.

## Watchouts

- Keep the work behavior-preserving.
- Do not broaden into `dispatch.cpp`, whole-call-family cleanup, phase
  extraction, ABI redesign, or expectation rewrites.
- Treat `materialize_call_boundary_source_to_destination` as non-same-shape
  despite being adjacent to before-call moves: it performs value publication
  from a frame-slot source into the call ABI destination, not only move-record
  sequencing.
- Any future absorption of scalar argument materialization, indirect callee
  lowering, call-result recording, missing frame-slot argument materialization,
  or stack-preservation publication should be a separate plan or reviewed
  route change with a wider proof surface.

## Proof

Delegated no-code audit proof: `git diff --check`.

Result: passed. No build or `test_after.log` was required for this packet.
