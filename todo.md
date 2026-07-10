Status: Active
Source Idea Path: ideas/open/674_rv64_object_terminator_lowering.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Select The Semantic Terminator-Lowering Boundary

# Current Packet

## Just Finished

Step 2 selected the semantic implementation owner without implementation,
test, expectation, marker, allowlist, runtime-policy, baseline, `plan.md`, or
source-idea edits. Evidence is under
`build/agent_state/674_step2_terminator_boundary/`.

The row 176 shape reaches the intended top-level path:
`fragment_for_prepared_terminator(...)` finds the prepared fused pointer branch
and calls `fragment_for_prepared_fused_pointer_branch(...)`. The current path
does not reject because of terminator dispatch, label relocation, compare
predicate support, prepared fact production, or missing condition/RHS
branch-stack-load authority.

The real rejection is in
`append_rv64_move_pointer_branch_operand_to_register(...)` for the LHS `%t10`.
`%t10` is a same-block `LoadLocalInst` pointer result from `main`/`entry` inst
4 with prepared memory access `base=frame_slot result=%t10 frame_slot=#8
offset=0 size=8 align=8`, and it is register-homed in `main` as value_id 15
(`reg=t0`). Existing pointer-source routes cover stack-carried sources,
materialized frame-slot pointer sources, and direct-global pointer sources, but
not a plain same-block local-memory load result. After those routes return
not-applicable, `block_defines_named_pointer_before(...)` sees that `%t10` is
defined before the terminator and returns `false` before the generic
register/formal-stack fallback can use it.

Selected owner for Step 3: add a narrowly scoped same-block `LoadLocalInst`
pointer operand materialization rule inside the fused pointer branch operand
helper, before the same-block-definition guard. The rule should identify a
unique prior same-block load-local producer for the requested pointer operand,
require function-local value-home/value-id agreement, require prepared
memory-access facts for that exact block/instruction/result, and emit the value
into the branch scratch register using existing prepared local-memory/frame-slot
semantics. This keeps the repair semantic and avoids row-name or testcase-shape
matching.

## Suggested Next

Execute Step 3 by implementing the same-block load-local pointer branch operand
source in `src/backend/mir/riscv/codegen/object_emission.cpp`, preferably as a
small helper called by `append_rv64_move_pointer_branch_operand_to_register(...)`
after the existing stack-carried/materialized/direct-global pointer source
routes and before `block_defines_named_pointer_before(...)`. Add focused
internal coverage near the existing fused pointer branch object tests for the
condition-plus-RHS-stack branch whose LHS is a same-block load-local pointer
result, plus fail-closed variants for missing, ambiguous, or mismatched
load-local facts.

## Watchouts

- Keep row 139 out of this route; it belonged to idea 673.
- Keep row 256 `backend_riscv_object_emission` as a guard surface.
- Do not edit expectations, unsupported markers, allowlists, runtime policy, or
  baseline accounting.
- Reject testcase-shaped lowering tied only to the row 176 test name.
- Keep malformed/sibling shapes fail-closed: no unique prior same-block
  `LoadLocalInst`; producer result spelling/type/value-id/home mismatch;
  homonymous values from another function such as the unrelated `%t10` in
  `writeback_callee`; missing/ambiguous/volatile/wrong-size/wrong-address-space
  or non-frame-slot prepared local-memory facts; producer at or after the
  terminator; multiple candidate loads; unsupported binary/select/cast/phi/call
  or global-load producers not already owned by an existing semantic route; and
  unavailable required branch stack-load authority.
- Guard row 256 must remain a guard, not the active owner.
- Do not broaden `fragment_for_prepared_terminator(...)`: it already dispatches
  row 176 into the fused pointer branch path.

## Proof

Inspection-only packet; no implementation proof was delegated or run.
Evidence captures these inspection commands under
`build/agent_state/674_step2_terminator_boundary/`:

- `rg -n "fragment_for_prepared_terminator|fragment_for_prepared_fused_pointer_branch|append_rv64_move_pointer_branch_operand_to_register|BranchStackLoad|branch stack|stack-load|stack_load" src/backend/mir/riscv/codegen/object_emission.cpp`
- `sed -n '4315,4390p' src/backend/mir/riscv/codegen/object_emission.cpp`
- `sed -n '13601,13725p' src/backend/mir/riscv/codegen/object_emission.cpp`
- `sed -n '13726,13985p' src/backend/mir/riscv/codegen/object_emission.cpp`
- `sed -n '2499,2648p' src/backend/prealloc/publication_plans.cpp`
- `sed -n '2677,3020p' src/backend/mir/riscv/codegen/prepared_local_memory_emit.cpp`
- `rg -n "home %t10|home %t12|home %t13|branch_condition entry|branch_stack_load_authority function=main block=entry|access block=entry inst_index=4|address_materialization block=entry inst_index=5|storage %t10|storage %t12|storage %t13" build/agent_state/674_step1_terminator_evidence/dump_prepared_bir.txt`

Suggested Step 3 proof command:
`cmake --build --preset default --target c4cll backend_riscv_object_emission_test -j 2`
then
`ctest --test-dir build -j --output-on-failure -R '^(backend_obj_runtime_rv64_indirect_store_postincrement_callee_contract|backend_riscv_object_emission)$' > test_after.log 2>&1`.
