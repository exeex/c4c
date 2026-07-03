# Current Packet

Status: Active
Source Idea Path: ideas/open/566_rv64_large_offset_gpr_callee_saved_frame_slots.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Inspect Large-Offset GPR Boundary

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

Advance to Step 2 with focused coverage plus minimal RV64 consumer repair for
large-offset GPR callee-saved save/restore materialization. Cover a validated
`gpr:s1` saved slot at `stack80000`, preserve existing direct-offset GPR
behavior, and keep missing or malformed prepared slot facts fail-closed.

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
