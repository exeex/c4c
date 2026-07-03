# Current Packet

Status: Active
Source Idea Path: ideas/open/566_rv64_large_offset_gpr_callee_saved_frame_slots.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Add Focused Large-Offset GPR Coverage

## Just Finished

Completed Step 1, `Inspect Large-Offset GPR Boundary`, for
`ideas/open/566_rv64_large_offset_gpr_callee_saved_frame_slots.md`.

The fresh representative probe for `src/20030209-1.c` still fails before object
emission with:

`unsupported_stack_frame: RV64 object route requires supported prepared callee-saved save slots`

The prepared facts are present and coherent before RV64 rejects the shape. The
fresh prepared dump records `prepared.summary @main stable_base=rsp
frame_size=80000 frame_alignment=8 saved_regs=3`, including `saved gpr:s1
order=0 placement=gpr:callee_saved#0/w1 width=1 units=s1
slot_placement=slot#10000+stack80000 slot_size=8 slot_align=8
fixed_location=yes slot_reg=gpr:s1 slot_save_index=0 slot_width=1
slot_units=s1 slot_register_placement=gpr:callee_saved#0/w1`.

The owning rejection remains RV64 consumer materialization, not prepared frame
publication. `src/backend/mir/riscv/codegen/object_emission.cpp`
`prepared_function_to_object_function(...)` calls
`make_rv64_prepared_call_frame_prologue_fragment(...)`; that returns no
fragment because `src/backend/mir/riscv/codegen/prepared_frame_emit.cpp`
`append_rv64_prepared_saved_callee_gpr_spills(...)` depends on
`rv64_prepared_saved_callee_gpr_stack_offset(...)`, which rejects the otherwise
valid `stack80000` slot because the helper only accepts direct signed 12-bit
stack immediates.

## Suggested Next

Step 2 executor packet: add focused RV64 backend coverage for large-offset GPR
callee-saved save/restore materialization, then apply the minimal consumer
repair needed for that coverage if the current implementation cannot express
the contract.

Scope:

- Primary file: `tests/backend/mir/backend_riscv_object_emission_test.cpp`.
- Implementation files only if needed by the focused tests:
  `src/backend/mir/riscv/codegen/prepared_frame_emit.cpp`,
  `src/backend/mir/riscv/codegen/object_emission.cpp`, and directly adjacent
  RV64 scalar/frame helpers.
- Cover a validated `gpr:s1` callee-saved slot at `stack80000`.
- Assert the save and restore path consumes prepared saved-register and
  prepared slot facts, rather than testcase names, raw register names, or
  synthesized frame facts.
- Preserve existing direct-offset GPR saved-register behavior.
- Keep missing or malformed prepared slot facts fail-closed.

Proof required:

- Build and backend tests:
  `cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_' > test_after.log`
- Representative probe after repair:
  `printf '%s\n' src/20030209-1.c > build/agent_state/566_step2_large_offset_gpr.allowlist && ALLOWLIST=build/agent_state/566_step2_large_offset_gpr.allowlist VERBOSE_FAILURES=1 scripts/check_progress_rv64_gcc_c_torture_backend.sh > build/agent_state/566_step2_large_offset_gpr.log 2>&1`
- Update `todo.md` with the old diagnostic status, backend proof result, and
  any downstream residual owner if the representative row still fails.

## Watchouts

- Leave `review/557_step13_vector_local_memory_review.md` untouched.
- Do not fold FPR callee-saved work back into this GPR large-offset idea.
- Do not synthesize prepared frame facts in RV64 lowering.
- Do not treat expectation, allowlist, or unsupported-marker edits as progress.
- A plain relaxation of the offset validator is not enough unless the emitted
  save/restore path materializes a large stack address through an explicit RV64
  addressing sequence.

## Proof

- Representative probe:
  `printf '%s\n' src/20030209-1.c > build/agent_state/566_step1_large_offset_gpr_boundary.allowlist && ALLOWLIST=build/agent_state/566_step1_large_offset_gpr_boundary.allowlist VERBOSE_FAILURES=1 scripts/check_progress_rv64_gcc_c_torture_backend.sh > build/agent_state/566_step1_large_offset_gpr_boundary.log 2>&1`
- Result: expected failure, `total=1 passed=0 failed=1`.
- Prepared dump:
  `build/c4cll --target riscv64-linux-gnu --dump-prepared-bir --mir-focus-function main tests/c/external/gcc_torture/src/20030209-1.c > build/agent_state/566_step1_20030209_prepared.txt 2> build/agent_state/566_step1_20030209_prepared.err`
- Fact excerpt: `build/agent_state/566_step1_large_offset_gpr_facts.txt`.
- No `test_after.log` was written; this packet was inspection-only.
