# Current Packet

Status: Active
Source Idea Path: ideas/open/727_common_prepared_return_chain_authority.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Audit the stale production inputs

## Just Finished

- Plan Step 1 audited the common classifier, regalloc producer, the synthetic
  one-link/two-link contract builder, and the representative AArch64
  return-lowering builder. The first shared bad fact is the first successor
  `PreparedMoveBundle::proof_attribution_id`: the contract builder's
  `chain_bundle` publishes a nonzero ID, while
  `prepared_with_return_selected_scalar_chain` aggregate-initializes both its
  one-link prefix and second link with zero. The classifier therefore returns
  `Stale` at the first `BeforeInstruction` bundle, before freshness selection;
  the same result applies to the one-link prefix and full multi-link walk.
- The exact production owner is the common prepared value-location builder:
  `build_prepared_value_location_function` -> `append_prepared_move_bundle` in
  `src/backend/prealloc/regalloc.cpp`. That path already derives a nonzero ID
  when it creates a bundle and preserves `function_name`, `phase`,
  `block_index`, `instruction_index`, and the exact `moves` element. Manual
  prepared-module builders bypass that owner. The bounded repair should make
  this producer authority reusable/mandatory for all prepared move-bundle
  construction, rather than teaching classification or AArch64 to synthesize
  it.
- Complete authority also requires each chain move to retain nonzero
  `from_value_id`/`to_value_id`, `op_kind == Move`, destination `Value`, and its
  owning bundle identity; the terminal `BeforeReturn` bundle must carry its own
  nonzero attribution plus the matching `FunctionReturnAbi` register move and
  `PreparedAbiBinding`. Source freshness need not encode target policy: derive
  it in common code from the attributed bundle's exact move and the complete
  source `PreparedValueHome` (`DirectHome`, `MoveBundleSource`,
  `DominanceOrOrdering`) with matching value name/id and block/instruction
  reference, as `publish_prepared_move_bundle_source_home_freshness_authorities`
  already does.

## Suggested Next

- Execute bounded Plan Step 2 in the common prepared producer seam: expose or
  centralize attributed move-bundle creation so both regalloc production and
  representative prepared-module builders publish the required bundle/move
  and terminal-binding fields, then prove one-link and multi-link traversal
  classify `Available`. Candidate owned files:
  `src/backend/prealloc/regalloc.cpp`,
  `src/backend/prealloc/value_locations.hpp`, and only the focused common and
  AArch64 return-chain tests needed to exercise the shared builder.

## Watchouts

- Preserve fail-closed distinctions: zero/mismatched attribution or mismatched
  function/position and a missing/mismatched direct-home freshness reference
  remain `Stale`; missing bundles/moves remain `Absent`; duplicate matching
  moves/bindings remain `Ambiguous`; invalid move IDs/op kinds remain
  `Unsupported`; missing terminal attribution/binding remains
  `StructurallyIncomplete`; inconsistent homes, non-adjacent producers, wrong
  chain operands, missing first-operand homes, and cycles retain their current
  precise negative statuses.
- Do not derive attribution from AArch64 register names, opcode sequences, test
  names, or fixed chain length. A deterministic producer-owned identity must
  be nonzero and distinct per published bundle; freshness must reference that
  bundle's exact move and common value home. Keep AArch64 implementation
  unchanged and avoid fixture-only injection.
- `backend_aarch64_instruction_dispatch` is the recorded baseline failure;
  `backend_aarch64_return_lowering` and the external add/sub-chain smoke failure
  remain blocker evidence in `test_after.log`.

## Proof

- Read-only audit: no build or test command was run, as delegated. Existing
  `test_after.log` records the representative multi-link failures in
  `backend_aarch64_return_lowering` and
  `backend_cli_aarch64_asm_external_return_add_sub_chain_smoke`; the matching
  prior evidence remains in `test_before.log`. Fresh implementation proof and
  a new `test_after.log` belong to Step 2.
