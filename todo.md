Status: Active
Source Idea Path: ideas/open/653_stack_carried_pointer_source_publication_materialization.md
Source Plan Path: plan.md
Current Step ID: 3A
Current Step Title: Publish LIR-to-BIR Compare Pointer Sources

# Current Packet

## Just Finished

Step 3A, `Publish LIR-to-BIR Compare Pointer Sources`: implemented a general
LIR-to-BIR compare-operand source publication path for tracked local-frame
pointer operands.

- `src/backend/bir/lir_to_bir/memory/coordinator.cpp` now materializes tracked
  local pointer compare operands from existing local pointer/address facts
  before scalar compare lowering consumes them.
- Focused BIR coverage in
  `tests/backend/bir/backend_lir_to_bir_notes_test.cpp` checks the positive
  local `[2 x i16]` compare source, a non-local pointer compare negative, and
  an out-of-range local array fail-closed shape.
- Fresh `20140828-1.c` semantic BIR now contains `%t6 = bir.add ptr %lv.a.0, 2`
  before `%t7 = bir.ne ptr %t4, %t6`.
- Fresh prepared output now contains an explicit `%t6` frame-slot address
  materialization fact: `address_materialization block=entry inst_index=2
  kind=frame_slot result=%t6 ... offset=6`.
- Evidence is in
  `build/agent_state/653_step3a_lir_to_bir_compare_pointer_sources/summary.md`.

## Suggested Next

Executor packet for Step 3B: connect the new Step 3A `%t6` local-frame pointer
producer/address-materialization fact through prepared/RV64 branch consumption.
The first downstream owner is the representative `20140828-1.c` object route
now failing closed with `unsupported_branch_stack_load_authority` /
`authority_status=missing_stack_clobber_safety`.

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
- The real `%t6` producer is no longer missing in semantic BIR; do not undo the
  Step 3A producer by moving the compare back to stack-slot-only inference.
- The representative object route now fails closed after Step 3A at
  `missing_stack_clobber_safety`, with `%t6` explicit and
  `source_freshness_status=selected`.
- Stack-slot-only preservation remains valid as ordinary stack preservation; it
  is not a pointer source materialization authority.
- `backend_prepare_frame_stack_call_contract` still exits later at the known
  `rv64 FPR ABI/frame fact contract` dump assertion, so use the new check's
  placement before that assertion when evaluating focused coverage.

## Proof

`test_after.log`: `cmake --build --preset default && ctest --test-dir build -j
--output-on-failure -R '^backend_'`.

Result: build completed and the delegated backend subset remains red with 32
failed tests out of 365. `backend_lir_to_bir_notes` passed in the delegated
subset. `test_after.log` is the preserved proof log.

Additional focused probes:

- `build/c4cll --target riscv64-linux-gnu --dump-bir
  tests/c/external/gcc_torture/src/20140828-1.c` wrote
  `build/agent_state/653_step3a_lir_to_bir_compare_pointer_sources/after_bir.txt`.
- `build/c4cll --target riscv64-linux-gnu --dump-prepared-bir
  tests/c/external/gcc_torture/src/20140828-1.c` wrote
  `build/agent_state/653_step3a_lir_to_bir_compare_pointer_sources/after_prepared.txt`.
- Focused `backend_lir_to_bir_notes` passed; focused
  `backend_prepare_frame_stack_call_contract` and
  `backend_riscv_object_emission` remain at known existing failures in
  `build/agent_state/653_step3a_lir_to_bir_compare_pointer_sources/focused_ctest.log`.
- Focused `20140828-1.c` object probe now fails at object compile with
  `unsupported_branch_stack_load_authority` /
  `authority_status=missing_stack_clobber_safety`; log:
  `build/agent_state/653_step3a_lir_to_bir_compare_pointer_sources/20140828_object_probe.log`.
