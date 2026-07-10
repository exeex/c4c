Status: Active
Source Idea Path: ideas/open/674_rv64_object_terminator_lowering.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Prove Regression Safety And Close Readiness

# Current Packet

## Just Finished

Step 3 implemented the focused same-block `LoadLocalInst` pointer branch
operand source in
`src/backend/mir/riscv/codegen/object_emission.cpp`. The new helper is called
from `append_rv64_move_pointer_branch_operand_to_register(...)` after the
existing stack-carried/materialized/direct-global pointer source routes and
before `block_defines_named_pointer_before(...)`, so row 176's LHS `%t10`
can be materialized from its prepared local frame-slot load instead of being
rejected only because it is defined earlier in the same block.

The route is semantic and fail-closed: it requires a unique prior same-block
`LoadLocalInst` pointer producer, matching result spelling/type, matching
prepared value name/id/home/function, exact indexed prepared memory-access
facts for the producer block/instruction/result, default nonvolatile frame-slot
base-plus-offset addressing, 8-byte size, at-least-8-byte alignment, and an
encodable prepared frame-slot stack offset. It rejects missing, ambiguous,
mismatched, volatile, wrong-address-space, wrong-size, non-frame-slot, or
post-terminator producer shapes.

Evidence is under
`build/agent_state/674_step3_terminator_lowering/`. Before evidence from Step
1 recorded row 176 failing with `unsupported_terminator_fragment` while row 256
passed; after this packet, row 176 and row 256 both pass the focused proof and
the comparable two-row regression guard.

## Suggested Next

Execute Step 4 with the supervisor-selected broader regression-safety proof and
then ask the plan owner to decide whether idea 674 is ready to close. Treat the
current row 176 improvement as focused proof only until the Step 4 gate is run.

## Watchouts

- Keep row 139 out of this route; it belonged to idea 673.
- Keep row 256 `backend_riscv_object_emission` as a guard surface.
- Do not edit expectations, unsupported markers, allowlists, runtime policy, or
  baseline accounting.
- Reject testcase-shaped lowering tied only to the row 176 test name.
- The Step 3 helper intentionally does not own binary/select/cast/phi/call or
  global-load same-block pointer producers; those remain unsupported unless an
  existing semantic source route owns them.
- Guard row 256 must remain a guard, not the active owner.
- Do not broaden `fragment_for_prepared_terminator(...)`: it already dispatches
  row 176 into the fused pointer branch path.

## Proof

Delegated proof passed and wrote canonical `test_after.log`:

`cmake --build --preset default --target c4cll backend_riscv_object_emission_test -j 2 && ctest --test-dir build -j --output-on-failure -R '^(backend_obj_runtime_rv64_indirect_store_postincrement_callee_contract|backend_riscv_object_emission)$' > test_after.log 2>&1`

Result:

- Row 176 `backend_obj_runtime_rv64_indirect_store_postincrement_callee_contract`: passed.
- Row 256 `backend_riscv_object_emission`: passed.
- Comparable before evidence: `build/agent_state/674_step3_terminator_lowering/test_before.log`.
- After evidence: `build/agent_state/674_step3_terminator_lowering/test_after.log`.
- Regression guard result: passed (`passed=1 failed=1 total=2` before,
  `passed=2 failed=0 total=2` after), resolving row 176 with no new failures.
