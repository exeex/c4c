# Execution State

Status: Active
Source Idea Path: ideas/open/81_convert_reviewed_x86_codegen_drafts_to_implementation_for_phoenix_rebuild.md
Source Plan Path: plan.md
Current Step ID: 2.2
Current Step Title: Migrate Canonical Call And Return Families
Plan Review Counter: 5 / 6
# Current Packet

## Just Finished

Completed step 2.2’s next non-call ownership shrink by moving the typed
load/store and atomic-loop helper family out of dormant
`shared_call_support.cpp` and into the compiled
`core/x86_codegen_output.cpp` seam: `reg_for_type`,
`mov_load_for_type`, `mov_store_for_type`, `type_suffix`, and
`emit_x86_atomic_op_loop`. The compiled seam now carries its own register-width
normalization helpers so this family no longer depends on dormant `mod.cpp`
ownership.

## Suggested Next

Keep step 2.2 on the remaining shared operand-pair cluster still owned only by
dormant `shared_call_support.cpp`: classify and migrate the next coherent
family among `operand_to_rcx`, `operand_to_rax_rdx`, and `prep_i128_binop`
through the compiled seam without widening into prepared-route or ABI-policy
work.

## Watchouts

- The backend target still does not compile `shared_call_support.cpp` or
  `mod.cpp`; seam moves have to remain self-contained in compiled reviewed
  owners rather than re-linking dormant utilities.
- This packet intentionally left the operand-pair/i128 prep helpers in the
  dormant draft path because they are the next coherent family, not part of
  the typed move/atomic cluster.
- Keep `call_lowering.cpp`, `memory.cpp`, and `mod.cpp` non-owning for this
  route; do not widen into ABI policy, prepared-route admission logic, or
  dormant utility reattachment.

## Proof

Step 2.2 typed move/atomic helper migration on 2026-04-22:
`cmake --build --preset default`
`ctest --test-dir build -j --output-on-failure -R '^backend_' > test_after.log`
Backend subset passed (`106/106`). Canonical log paths: `test_before.log`,
`test_after.log`
