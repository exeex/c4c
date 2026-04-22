# Execution State

Status: Active
Source Idea Path: ideas/open/81_convert_reviewed_x86_codegen_drafts_to_implementation_for_phoenix_rebuild.md
Source Plan Path: plan.md
Current Step ID: 2.2.2
Current Step Title: Migrate Canonical Call Issuance, Cleanup, And Result Publication
Plan Review Counter: 0 / 6
# Current Packet

## Just Finished

Completed the last observed packet for new step 2.2.1 by moving
`operand_to_rcx`, `operand_to_rax_rdx`, and `prep_i128_binop` out of dormant
`shared_call_support.cpp` and into the compiled
`core/x86_codegen_output.cpp` seam. Canonical scalar, comparison, i128, and
return helpers now consume the pair-loader/i128 prep path from the reviewed
compiled owner instead of leaving that family owned only by the dormant draft
file.

## Suggested Next

Advance step 2.2.2 on canonical call/return ownership only: classify the next
coherent helper family still living in dormant or mixed legacy support around
call issuance, cleanup, or result publication, and migrate it behind reviewed
lowering or compiled owners without widening into prepared-route admission or
ABI-policy work.

## Watchouts

- The backend target still does not compile `shared_call_support.cpp` or
  `mod.cpp`; seam moves have to remain self-contained in compiled reviewed
  owners rather than re-linking dormant utilities.
- The operand-pair/i128 prep helpers now live in the compiled seam, but
  `shared_call_support.cpp` still carries other dormant support definitions; do
  not treat this packet as permission to revive that file as an owner.
- Keep `call_lowering.cpp`, `memory.cpp`, and `mod.cpp` non-owning for this
  route; do not widen into ABI policy, prepared-route admission logic, or
  dormant utility reattachment.

## Proof

Step 2.2.1 operand-pair/i128 prep helper migration on 2026-04-22:
`cmake --build --preset default`
`ctest --test-dir build -j --output-on-failure -R '^backend_' > test_after.log`
Backend subset passed (`106/106`). Canonical log paths: `test_before.log`,
`test_after.log`
