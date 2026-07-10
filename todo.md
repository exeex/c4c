Status: Active
Source Idea Path: ideas/open/674_rv64_object_terminator_lowering.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Refresh Row 176 Terminator Evidence

# Current Packet

## Just Finished

Step 1 refreshed row 176 evidence without implementation or expectation edits.
`backend_obj_runtime_rv64_indirect_store_postincrement_callee_contract` still
fails during `--codegen obj` with
`unsupported_terminator_fragment: BIR terminator requires unsupported RV64
object lowering`, while guard row 256 `backend_riscv_object_emission` passes.
Evidence is under `build/agent_state/674_step1_terminator_evidence/`.

The exact unsupported row 176 terminator is in `main`, block `entry`:
`%t10 = bir.load_local ptr %lv.slots.2`; `%t12 = bir.add ptr
%lv.values.0, 8`; `%t13 = bir.ne ptr %t10, %t12`; `bir.cond_br i32
%t13, block_4, block_5`. Prepared control flow classifies it as
`kind=fused_compare`, `compare=ne ptr %t10, %t12`, `can_fuse_with_branch=yes`,
true target `block_4`, false target `block_5`.

Prepared branch stack-load authority exists for `main`/`entry` at terminator
instruction index 7: role `condition` value `%t13` value_id 17, slot `#11`,
stack offset 48, selected `BranchStackLoadSource`; and role `rhs` value `%t12`
value_id 16, slot `#10`, stack offset 40, selected `BranchStackLoadSource`.
`%t10` is loaded from frame slot `#8` at entry inst 4, and `%t12` is an address
materialization for `%lv.values.0 + 8` at entry inst 5.

## Suggested Next

Execute Step 2 by inspecting RV64 object terminator lowering in
`src/backend/mir/riscv/codegen/object_emission.cpp`, especially
`fragment_for_prepared_terminator(...)`,
`fragment_for_prepared_fused_pointer_branch(...)`, and
`append_rv64_move_pointer_branch_operand_to_register(...)`, to determine why
this condition-plus-RHS-stack fused pointer branch does not pass the existing
semantic fragment path.

## Watchouts

- Keep row 139 out of this route; it belonged to idea 673.
- Keep row 256 `backend_riscv_object_emission` as a guard surface.
- Do not edit expectations, unsupported markers, allowlists, runtime policy, or
  baseline accounting.
- Reject testcase-shaped lowering tied only to the row 176 test name.
- The row 176 failure is not in prepared fact production: the prepared dump
  already publishes condition and RHS branch stack-load authority. The next
  boundary is RV64 object terminator consumption/admission or operand
  materialization for the real prepared shape.
- Existing internal object tests already cover related fused pointer branch
  shapes, so Step 2 should compare the real row 176 prepared facts against
  those fixtures before changing code.

## Proof

Build command:
`cmake --build --preset default --target c4cll backend_riscv_object_emission_test -j 2`
passed with no work to do.

Focused proof command:
`ctest --test-dir build -j --output-on-failure -R '^(backend_obj_runtime_rv64_indirect_store_postincrement_callee_contract|backend_riscv_object_emission)$' > test_after.log 2>&1`
completed with expected exit code 8 because row 176 failed and row 256 passed.
`test_after.log` is the canonical proof log.

Evidence commands captured `--codegen obj`, `--dump-bir`, full
`--dump-prepared-bir`, and focused `--dump-prepared-bir --mir-focus-function
writeback_callee` outputs under
`build/agent_state/674_step1_terminator_evidence/`.
