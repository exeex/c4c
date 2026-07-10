Status: Active
Source Idea Path: ideas/open/653_stack_carried_pointer_source_publication_materialization.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Implement The Narrow Stack-Carried Pointer Rule

# Current Packet

## Just Finished

Step 3, `Implement The Narrow Stack-Carried Pointer Rule`: added the RV64
consumer for explicit stack-carried pointer source authority in prepared fused
pointer branch operands.

- Changed files:
  `src/backend/mir/riscv/codegen/object_emission.cpp` and
  `tests/backend/mir/backend_riscv_object_emission_test.cpp`.
- RV64 now checks a branch operand's latest prior stack-slot preservation before
  falling back to ordinary operand moves. If that preservation exists, the
  consumer requires an explicit `source_selection` matching the preserved stack
  home and a validated `LocalFrameAddressMaterialization` with
  `PointerBasePlusOffset` source identity.
- When the fact is present, RV64 materializes the source local-frame pointer
  address directly from the prepared offset instead of reloading the carrier
  stack slot. Missing, mismatched, stack-slot-only, or stale shapes stay
  fail-closed.
- Focused object-emission coverage builds a synthetic `%t6`-class pointer
  branch that materializes `addi rd, sp, offset` from the explicit preserved
  source selection, plus fail-closed mutations for absent and contradictory
  authority.
- Evidence written to
  `build/agent_state/653_step3_rv64_stack_carried_pointer_consumer/summary.md`.

## Suggested Next

Executor packet: repair the producer/hand-off gap for the real
`src/20140828-1.c` `%t6` row so the stack-slot preserved value for value id
`19`, slot `#16`, carries the explicit `source_selection` fact currently proven
by the synthetic RV64 consumer test.

## Watchouts

- Do not reopen RV64 terminator-fragment admission from idea 645.
- Do not infer pointer freshness or materialization from stack offsets, final
  assembly shape, source spelling, local names, diagnostics, testcase identity,
  runtime outcomes, or pass/fail accounting.
- `loop-2e.c` now passes the direct runtime runner; do not use `%t23` as the
  first failing runtime proof unless a later packet identifies a still-red
  focused owner.
- The RV64 consumer now rejects prior stack-slot preservation without explicit
  source selection for pointer branch operands. This intentionally blocks the
  old `%t6` stack-slot-only path instead of emitting the previous runtime
  aborting object.
- Current focused `src/20140828-1.c` prepared output still has no
  `source_selection` on `preserve value=%t6 value_id=19 route=stack_slot
  slot=#16 stack_offset=8`; the remaining owner is producer/hand-off
  attachment for that real row, not RV64 inference.
- Stack-slot-only preservation remains valid as ordinary stack preservation; it
  is not a pointer source materialization authority.
- `backend_prepare_frame_stack_call_contract` still exits later at the known
  `rv64 FPR ABI/frame fact contract` dump assertion, so use the new check's
  placement before that assertion when evaluating focused coverage.

## Proof

`test_after.log`: `cmake --build --preset default && ctest --test-dir build -j
--output-on-failure -R '^backend_'`.

Result: build completed and the delegated backend subset remains red with 32
failed tests out of 365. `test_after.log` is the preserved proof log.

Additional focused proof: `cmake --build --preset default --target
backend_riscv_object_emission_test` builds. Running
`build/tests/backend/mir/backend_riscv_object_emission_test` still prints
unrelated existing failures, but the new stack-carried pointer consumer checks
do not add failure output.

Focused `src/20140828-1.c` probe:
`tests/backend/cmake/run_rv64_gcc_torture_backend_object_case.cmake` now fails
at object compile with `unsupported_terminator_fragment` because the current
prepared dump lacks the real `%t6` preserved `source_selection`; this replaces
the previous bad runtime-aborting object path and identifies the remaining
producer/hand-off owner.
