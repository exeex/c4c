Status: Active
Source Idea Path: ideas/open/30_riscv_prepared_edge_publication_stack_destination_policy_broadening.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Inventory RISC-V Stack-Destination Source Options

# Current Packet

## Just Finished

Step 1 inventory completed for the RISC-V prepared edge-publication
stack-destination route. Target files/functions are
`src/backend/mir/riscv/codegen/emit.hpp` (`EdgePublicationMoveIntent`) and
`src/backend/mir/riscv/codegen/emit.cpp`
(`render_edge_publication_source_operand`,
`consume_edge_publication_move_intent`, and
`append_edge_publication_move_instruction`), with focused coverage in
`tests/backend/bir/backend_riscv_prepared_edge_publication_test.cpp`
(`check_register_to_stack_slot_move_uses_shared_lookup`,
`check_stack_destination_fail_closed_forms`,
`check_missing_and_unsupported_homes_fail_closed`, and the existing
register-destination source-form checks).

Available source facts from shared prepared lookup authority are source home
kind, optional source value id, register name, `immediate_i32`, stack slot id,
stack offset, stack size, pointer base value name, pointer byte delta, and
resolved pointer base register when the base home is a register. Available
destination facts are destination home kind, destination value id, stack slot
id, stack offset, and stack size. Existing supported stack-destination policy
is exactly `Register -> StackSlot` for concrete 4-byte stack destinations,
emitting `sw <source-register>, <offset>(sp)` through the shared
`edge_publications` lookup.

Policy blocker recorded instead of selecting a broadened implementation form:
`RematerializableImmediate -> StackSlot`, `StackSlot -> StackSlot`, and
`PointerBasePlusOffset -> StackSlot` all require a RISC-V scratch register or
scratch-reservation rule inside the prepared edge-publication consumer before
they can be emitted without guessing. The current helper has no owned scratch
register contract for stack-destination edge moves; using a hard-coded scratch
register here would be a target-local policy decision not yet represented by
the helper's inputs. Keep those forms fail-closed for the next packet unless
the supervisor explicitly authorizes a concrete scratch policy.

## Suggested Next

Execute Step 2 as an explicit fail-closed policy packet: preserve
`Register -> StackSlot`, keep non-register source-to-`StackSlot` forms
unsupported, and add or tighten focused assertions documenting the scratch
policy blocker without broadening stack-source register-destination or
pointer-base register-destination behavior.

## Watchouts

Fail-closed source-to-stack forms are `RematerializableImmediate -> StackSlot`,
`StackSlot -> StackSlot`, and `PointerBasePlusOffset -> StackSlot`; malformed
stack destinations, non-move publications, missing shared lookup authority,
missing source homes, unsupported source facts, and missing publication facts
also remain fail-closed. Do not broaden stack-source register-destination
policy or pointer-base register-destination policy in this plan. Preserve
shared `edge_publications` as the only semantic authority for supported
stack-destination moves.

## Proof

Passed:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_riscv_prepared_edge_publication$' > test_after.log 2>&1`.
Build reported `ninja: no work to do`; focused CTest passed 1/1 in
`test_after.log`. The delegated proof is sufficient for this inventory-only
Step 1 packet.
