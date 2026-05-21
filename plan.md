# AArch64 Pointer-Derived Load Address Scaling Timeout Runbook

Status: Active
Source Idea: ideas/open/362_aarch64_pointer_derived_load_address_scaling_timeout.md

## Purpose

Repair the AArch64 residual where `00181` now times out after materialized
pointer-addressed store writeback was repaired.

## Goal

Make pointer-derived loads or address scaling use the correct computed address
so `00181` advances beyond the 5 second timeout without reopening the repaired
store-writeback path.

## Core Rule

Localize and repair the general pointer-derived load/address scaling boundary.
Do not special-case `00181`, `Hanoi`, tower names, stack offsets, ABI
registers, one emitted instruction neighborhood, or the 5 second timeout
itself.

## Read First

- `ideas/open/362_aarch64_pointer_derived_load_address_scaling_timeout.md`
- `ideas/open/361_aarch64_materialized_pointer_storelocal_writeback.md`
- `ideas/open/360_aarch64_hanoi_starting_state_output_mismatch.md`
- `src/backend/mir/aarch64/codegen/dispatch.cpp`
- `tests/c/external/c-testsuite/src/00181.c`
- `tests/backend/mir/backend_aarch64_instruction_dispatch_test.cpp`

## Current Targets

- Primary external representative: `c_testsuite_aarch64_backend_src_00181_c`
- Stability representatives: `c_testsuite_aarch64_backend_src_00170_c`,
  `c_testsuite_aarch64_backend_src_00189_c`
- Backend contracts:
  `backend_aarch64_instruction_dispatch`,
  `backend_aarch64_memory_operand_contract`,
  `backend_prepare_frame_stack_call_contract`,
  `backend_cli_dump_prepared_bir_local_arg_call_contract`

## Non-Goals

- Do not reopen materialized pointer-addressed store writeback from idea 361
  except to keep it stable.
- Do not reopen the idea 360 direct `LoadGlobal` current-memory select-store
  repair except to keep it stable.
- Do not reopen stack-preserved pointer formal post-call handling from idea
  359.
- Do not reopen scalar formal post-call reloads from idea 358.
- Do not reopen pointer-formal callee-saved home publication from idea 357.
- Do not handle semantic-BIR dynamic pointer-derived string loads for `00173`.
- Do not handle frontend or semantic admission failure for `00005`.
- Do not change expectations, unsupported classifications, runner behavior,
  timeout policy, CTest registration, or proof-log policy.
- Do not broaden into ABI composite/byval/HFA/f128, variadic floating,
  dynamic stack, or unrelated scalar compare/select residuals.

## Working Model

After commit `ee027c36a`, `00181` advanced beyond the old unchanged
subsequent-state runtime mismatch. Generated `Move` assembly now reloads
computed pointers and emits real stores through them before `PrintAll`. The
remaining failure is a 5 second timeout, with suspicious generated address
scaling such as `mov x9, #4; mul x9, x9, x9` around pointer-derived loads.
The likely owner is now on the load/address side rather than missing store
writeback.

## Execution Rules

- Use generated-code and prepared-BIR evidence to confirm the first bad
  pointer-derived load/address scaling fact before editing.
- Compare semantic BIR, prepared BIR, and generated AArch64 before choosing
  the owner boundary.
- Prefer existing memory, local, GEP, address-projection, and load-address
  materialization helpers and local codegen patterns.
- Add focused backend coverage for the localized pointer-derived load/address
  scaling shape before claiming capability progress.
- Keep the idea 360 starting-state output correct.
- Keep the idea 361 materialized pointer store writeback evidence present.
- Keep ideas 357, 358, and 359 stable.
- Treat named-case matching, timeout-policy changes, or expectation weakening
  as route failure.

## Step 1: Localize The Pointer-Derived Load Scaling Boundary

Goal: find the first pointer-derived load or address-scaling operation that can
explain the post-writeback `00181` timeout.

Actions:

- Reproduce the focused subset including `00181`, `00170`, `00189`, and the
  backend contracts listed above.
- Inspect `00181.c`, semantic BIR, prepared BIR, and generated AArch64 around
  the repaired `Move` stores and the following load or recursion consumer.
- Identify the producing pointer value, index or scale carrier, expected
  address, emitted address calculation, load behavior, and timeout-causing
  consumer.
- Record the first bad fact and any adjacent same-shape examples in `todo.md`.

Completion check:

- `todo.md` names the source load operation, pointer/index carrier, expected
  address, emitted address calculation, first wrong boundary, and consumer
  path.
- The idea 361 store-writeback repair remains outside this step unless fresh
  evidence contradicts the split.

## Step 2: Repair The Pointer-Derived Load Address Rule

Goal: repair the general AArch64 lowering rule that computes the wrong address
for pointer-derived loads.

Actions:

- Update only the localized AArch64 load/address scaling boundary.
- Ensure the repair applies to the general pointer-derived load shape, not just
  `00181`, one stack offset, or one emitted multiply.
- Preserve materialized pointer-addressed store writeback from idea 361.
- Preserve direct `LoadGlobal` current-memory select-store handling from idea
  360.
- Preserve recursive formal post-call repairs from ideas 357, 358, and 359.
- Add focused backend coverage for the repaired load/address scaling shape.

Completion check:

- Focused backend coverage fails before the repair and passes after it.
- Generated code or execution evidence shows the representative load computes
  the expected pointer-derived address.
- `git diff --check` passes.

## Step 3: Prove Focused External Progress

Goal: prove the pointer-derived load/address scaling repair advances `00181`
without regressing nearby repaired paths.

Actions:

- Run:
  `cmake --build --preset default && ctest --test-dir build -j10 --output-on-failure -R '^(backend_aarch64_instruction_dispatch|backend_aarch64_memory_operand_contract|backend_prepare_frame_stack_call_contract|backend_cli_dump_prepared_bir_local_arg_call_contract|c_testsuite_aarch64_backend_src_00181_c|c_testsuite_aarch64_backend_src_00170_c|c_testsuite_aarch64_backend_src_00189_c)$' | tee test_after.log`
- If `00181` still fails, classify the new first bad fact in `todo.md` and
  split it only if it is outside the pointer-derived load/address scaling
  owner.
- Keep `00170`, `00189`, the idea 360 starting-state output, and the idea 361
  store writeback evidence stable.

Completion check:

- The delegated proof command is recorded in `todo.md`.
- `00181` advances beyond the 5 second timeout or has a newly localized
  out-of-scope first bad fact.
- Stability representatives remain passing.

## Step 4: Broader Backend Guard And Lifecycle Decision

Goal: decide whether the residual is complete and safe to close or whether a
new first bad fact needs its own source idea.

Actions:

- Compare the final residual, if any, against the source idea's in-scope and
  out-of-scope lists.
- Ask the supervisor for a broader backend guard before closure because the
  repair touches shared load/address lowering behavior.
- Split unrelated residuals into new `ideas/open/` source ideas instead of
  absorbing them into this plan.

Completion check:

- The pointer-derived load/address scaling residual closes only when it is
  repaired, guarded, and not masking a separate owner.
- Any unrelated residual is split into a new source idea.
