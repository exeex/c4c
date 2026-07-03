# Current Packet

Status: Active
Source Idea Path: ideas/open/564_rv64_fpr_callee_saved_frame_slots.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Repair RV64 FPR Callee-Saved Slot Consumption

## Just Finished

Executed Step 3, `Repair RV64 FPR Callee-Saved Slot Consumption`.

Changed files:

- `src/backend/mir/riscv/codegen/prepared_frame_emit.cpp`
- `src/backend/mir/riscv/codegen/prepared_scalar_emit.cpp`
- `src/backend/mir/riscv/codegen/object_emission.cpp`
- `tests/backend/mir/backend_riscv_object_emission_test.cpp`
- `todo.md`

Repair completed:

- RV64 prepared frame prologues now consume validated prepared FPR
  callee-saved slot facts and emit 8-byte `fsd` saves.
- RV64 prepared return epilogues now route through the prepared-frame epilogue
  helpers and emit matching 8-byte `fld` restores for validated FPR saved
  registers.
- The FPR materializer reuses
  `rv64_prepared_saved_callee_fpr_stack_offset(...)` for fact validation and
  handles both direct 12-bit stack offsets and larger validated offsets through
  a scratch base.
- The saved-register admission gate now permits coherent prepared GPR and FPR
  callee-saved register banks while keeping mismatched banks fail-closed.
- Focused backend coverage proves a two-register `fs1`/`fs2` FPR
  callee-saved save/restore sequence and keeps malformed FPR slot size
  rejected.

Representative probe:

- Ran
  `ALLOWLIST=build/agent_state/564_step3_fpr_callee_saved_after.allowlist VERBOSE_FAILURES=1 scripts/check_progress_rv64_gcc_c_torture_backend.sh > build/agent_state/564_step3_fpr_callee_saved_after.log 2>&1`.
- Result: `total=2 passed=0 failed=2`.
- `src/20000603-1.c` moved off the old non-GPR callee-saved save-slot
  diagnostic and now fails later at
  `unsupported_instruction_fragment: BIR instruction requires unsupported RV64 object lowering`.
- `src/20030209-1.c` also moved off the old non-GPR diagnostic text, but still
  fails stack-frame admission with
  `unsupported_stack_frame: RV64 object route requires supported prepared callee-saved save slots`.
  The remaining stack-frame blocker is the prepared `gpr:s1` saved slot at
  `stack80000`, which is outside the existing GPR saved-register offset
  contract and is distinct from FPR callee-saved materialization.

## Suggested Next

Execute Step 4, `Reconcile Representatives And Residual Owners`.

Executor packet:

- Objective: reconcile the two representative outcomes after FPR
  callee-saved materialization and decide whether the active idea can close or
  needs a separate residual idea for large-offset GPR saved-register frame
  slots.
- Owned files: `todo.md` and, only if necessary for classification evidence,
  `build/agent_state/564_step4_*` artifacts.
- Do not touch: implementation files, `plan.md`, source idea files,
  `review/`, expectations, unsupported markers, or representative allowlists.
- Required checks: confirm the old `non-GPR prepared callee-saved register
  save slots` diagnostic is gone from both representative logs, record the
  current downstream owner for each row, and recommend close/split/continue.
- Proof command:
  inspection/classification only unless the supervisor asks for another
  backend subset.
- Done when: `todo.md` records representative residual owners and the lifecycle
  recommendation for source idea `564`.

## Watchouts

- Leave prepared frame-layout production out of scope; current prepared dumps
  prove the FPR callee-saved slot facts are present.
- `prepared_scalar_emit.cpp` was touched because prepared return terminator
  epilogues still used a GPR-only restore wrapper; without that handoff, FPR
  prologue saves worked but return restores failed.
- `src/20030209-1.c` still has a stack-frame residual, but the immediate owner
  appears to be large-offset GPR callee-saved slot consumption
  (`gpr:s1 stack80000`), not FPR saved-register materialization.
- Do not turn Step 4 into a GPR large-frame repair unless the supervisor
  explicitly delegates that new packet or creates a separate source idea.

## Proof

Proof command:

`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_' > test_after.log`

Result: passed, `345/345` backend tests.

Log path: `test_after.log`.

Representative log:

`build/agent_state/564_step3_fpr_callee_saved_after.log`
