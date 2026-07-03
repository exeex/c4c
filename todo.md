# Current Packet

Status: Active
Source Idea Path: ideas/open/566_rv64_large_offset_gpr_callee_saved_frame_slots.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Add Focused Large-Offset GPR Coverage

## Just Finished

Completed Step 2, `Add Focused Large-Offset GPR Coverage`, for
`ideas/open/566_rv64_large_offset_gpr_callee_saved_frame_slots.md`.

Changed files:

- `src/backend/mir/riscv/codegen/prepared_frame_emit.cpp`
- `tests/backend/mir/backend_riscv_object_emission_test.cpp`
- `todo.md`

Added focused RV64 backend coverage for prepared `gpr:s1` callee-saved save
slots at both direct offset `24` and large offset `stack80000`, including
fail-closed checks for missing/mismatched prepared slot facts. The large-offset
object test asserts explicit materialization of `sp + 80000` before `sd s1,
0(t1)` and again before `ld s1, 0(t1)`.

Repaired the RV64 prepared-frame consumer so
`rv64_prepared_saved_callee_gpr_stack_offset(...)` validates coherent prepared
GPR save-slot facts without requiring a signed 12-bit offset, while GPR
save/restore emission now routes through the existing stack-offset store/load
helpers that materialize large stack addresses.

The representative probe for `src/20030209-1.c` no longer reports the old
`unsupported_stack_frame: RV64 object route requires supported prepared
callee-saved save slots` diagnostic. It now reaches downstream
`unsupported_instruction_fragment: BIR instruction requires unsupported RV64
object lowering`.

## Suggested Next

Plan-owner review/close decision for idea 566, or a new lifecycle packet for the
downstream `src/20030209-1.c` unsupported-instruction residual if the supervisor
chooses to continue that representative row.

## Watchouts

- Leave `review/557_step13_vector_local_memory_review.md` untouched.
- Do not fold FPR callee-saved work back into this GPR large-offset idea.
- Do not synthesize prepared frame facts in RV64 lowering.
- Do not treat expectation, allowlist, or unsupported-marker edits as progress.
- The `src/20030209-1.c` residual is no longer the large-offset GPR
  callee-saved slot. Its current owner is generic unsupported RV64 instruction
  lowering after the frame is accepted.

## Proof

- Backend proof:
  `cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_' > test_after.log`
- Result: passed; canonical proof log is `test_after.log`.
- Representative probe:
  `printf '%s\n' src/20030209-1.c > build/agent_state/566_step2_large_offset_gpr.allowlist && ALLOWLIST=build/agent_state/566_step2_large_offset_gpr.allowlist VERBOSE_FAILURES=1 scripts/check_progress_rv64_gcc_c_torture_backend.sh > build/agent_state/566_step2_large_offset_gpr.log 2>&1`
- Result: expected downstream failure, `total=1 passed=0 failed=1`; old
  `unsupported_stack_frame` diagnostic is gone and the current diagnostic is
  `unsupported_instruction_fragment`.
