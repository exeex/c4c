# Current Packet

Status: Active
Source Idea Path: ideas/open/564_rv64_fpr_callee_saved_frame_slots.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Inspect FPR Callee-Saved Boundary

## Just Finished

Executed Step 1, `Inspect FPR Callee-Saved Boundary`.

Fresh representative probe:

- Command:
  `printf '%s\n' src/20000603-1.c src/20030209-1.c > build/agent_state/564_step1_fpr_callee_saved_boundary.allowlist && ALLOWLIST=build/agent_state/564_step1_fpr_callee_saved_boundary.allowlist VERBOSE_FAILURES=1 scripts/check_progress_rv64_gcc_c_torture_backend.sh > build/agent_state/564_step1_fpr_callee_saved_boundary.log 2>&1`
- Result: expected failure, `total=2 passed=0 failed=2`.
- Both rows fail first with
  `unsupported_stack_frame: RV64 object route does not support non-GPR prepared callee-saved register save slots (fpr:fs1)`.

Prepared fact dumps:

- `build/agent_state/564_step1_20000603_prepared.txt`
- `build/agent_state/564_step1_20030209_prepared.txt`
- Compact fact extract:
  `build/agent_state/564_step1_fpr_callee_saved_facts.txt`

Prepared production is present before RV64 rejects:

- `src/20000603-1.c`: function `f` has `frame_size=16`,
  `saved fpr:fs1`, placement `fpr:callee_saved#0/w1`, save index `0`,
  slot `slot#11+stack24`, slot size `8`, slot align `8`, fixed location,
  and matching `slot_reg=fpr:fs1`.
- `src/20030209-1.c`: function `main` has `frame_size=80000`,
  `saved fpr:fs1` at `slot#10001+stack80008` and `saved fpr:fs2` at
  `slot#10002+stack80016`, both slot size `8`, slot align `8`, fixed
  location, and matching FPR slot registers.

Owning boundary is still RV64 consumer-owned:

- Admission callback wiring:
  `src/backend/mir/riscv/codegen/object_emission.cpp::prepared_function_to_object_function(...)`
  passes `diagnose_unsupported_prepared_saved_register_bank`.
- First rejection:
  `src/backend/mir/riscv/codegen/object_emission.cpp::diagnose_unsupported_prepared_saved_register_bank(...)`
  rejects `frame_plan->saved_callee_registers` entries whose `saved.bank` is
  not `Gpr`.
- Existing materialization helpers are GPR-only:
  `src/backend/mir/riscv/codegen/prepared_frame_emit.cpp::rv64_prepared_saved_callee_gpr_stack_offset(...)`,
  `append_rv64_prepared_saved_callee_gpr_spills(...)`, and
  `append_rv64_prepared_saved_callee_gpr_restores(...)`.

## Suggested Next

Advance to Step 2. Add focused RV64 backend coverage for prepared FPR
callee-saved frame-slot consumption using an explicit prepared `fpr:fs1` slot
shape. The coverage should assert that the prepared FPR saved-register fact,
slot offset, slot size, and save index are consumed as target facts, while
missing or malformed prepared FPR slot facts still fail closed. Do not repair
prepared frame production; the current dumps prove it is already publishing
the representative facts.

## Watchouts

- Leave prepared frame-layout production out of scope; current prepared dumps
  prove the FPR callee-saved slot facts are present.
- Do not special-case filenames, `fs1`, diagnostic text, unsupported markers,
  allowlists, or expected outputs.
- Preserve existing GPR callee-saved frame behavior while extending or testing
  the FPR path.
- `src/20030209-1.c` also publishes `fpr:fs2`; Step 2 can stay focused on
  the `fs1` representative shape but should avoid assuming only one FPR saved
  register can exist.

## Proof

Inspection-only packet; no `test_after.log` was written.

Commands/logs:

- Representative probe:
  `build/agent_state/564_step1_fpr_callee_saved_boundary.log`
- Prepared dump for `src/20000603-1.c`:
  `build/c4cll --target riscv64-linux-gnu --dump-prepared-bir tests/c/external/gcc_torture/src/20000603-1.c > build/agent_state/564_step1_20000603_prepared.txt 2> build/agent_state/564_step1_20000603_prepared.err`
- Prepared dump for `src/20030209-1.c`:
  `build/c4cll --target riscv64-linux-gnu --dump-prepared-bir tests/c/external/gcc_torture/src/20030209-1.c > build/agent_state/564_step1_20030209_prepared.txt 2> build/agent_state/564_step1_20030209_prepared.err`
- `git diff --check -- todo.md` passed after this update.
