Status: Active
Source Idea Path: ideas/open/653_stack_carried_pointer_source_publication_materialization.md
Source Plan Path: plan.md
Current Step ID: 3A
Current Step Title: Publish LIR-to-BIR Compare Pointer Sources

# Current Packet

## Just Finished

Step 3, `Implement The Narrow Stack-Carried Pointer Rule`: traced the real
`tests/c/external/gcc_torture/src/20140828-1.c` `%t6` / `&a[1]` producer gap
and stopped because the required semantic producer fact belongs before
prepared/prealloc.

Plan-owner review split the remaining route into Step 3A,
`Publish LIR-to-BIR Compare Pointer Sources`, followed by Step 3B,
`Connect Prepared And RV64 Stack-Carried Pointer Authority`. This stays inside
idea 653 because the missing compare-operand producer is the required upstream
authority for the same `%t6` stack-carried pointer publication chain, not a
separate initiative.

- Evidence written to
  `build/agent_state/653_step3_pointer_base_plus_offset_producer_fact/summary.md`.
- Prepared dump still shows `preserve value=%t6 value_id=19 route=stack_slot
  spill_slot=slot#16+stack8 ...` without `source_selection`.
- Semantic BIR for `main` compares `%t4` against `%t6`, but there is no
  defining `%t6 = %lv.a.0 + 2` instruction in the block.
- Prepared value homes still record `%t6` value id `19` as `kind=stack_slot
  slot_id=16 offset=8`, not `PointerBasePlusOffset`, because no pointer-carrier
  or address-materialization producer row exists for `%t6`.
- Branch stack-load authority proves only stack-slot freshness for `%t6`; it
  does not carry the semantic local-frame pointer source.
- The exact upstream owner is LIR-to-BIR pointer compare/source lowering:
  `src/backend/bir/lir_to_bir/memory/coordinator.cpp` dispatches `LirCmpOp` to
  scalar lowering, and `src/backend/bir/lir_to_bir/scalar.cpp`
  `lower_scalar_compare_inst` lowers compare operands via `lower_value`
  without emitting or publishing a named pointer producer for address-valued
  operands.

## Suggested Next

Executor packet for Step 3A: add a general LIR-to-BIR compare-operand pointer
source publication/materialization path for address-valued operands, so
`%t6`-class local-frame pointer operands are represented as named BIR producers
or explicit address materialization facts before prepared/prealloc builds value
homes and call preservation.

Required first proof target: focused BIR/LIR-to-BIR evidence for
`tests/c/external/gcc_torture/src/20140828-1.c` showing the semantic `%t6`
compare operand has an explicit `%lv.a.0 + 2` producer or equivalent
address-materialization fact, without reconstructing it later from stack homes.

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
  slot=#16 stack_offset=8`.
- The missing owner is upstream source publication for `%t6`: semantic BIR and
  prepared producer lookups do not contain a `%t6 = %lv.a.0 + 2` fact to hand
  off. Do not patch call preservation by reconstructing that fact from source
  spelling, stack offsets, final comparison shape, or testcase identity.
- A prepared value-home ordering patch was explored and rejected for this
  packet: it can preserve `PointerBasePlusOffset` plus stack-home data when a
  carrier already exists, but the real `%t6` has no carrier because BIR compare
  lowering never publishes the producer.
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

Additional focused probes:

- `build/c4cll --target riscv64-linux-gnu --dump-prepared-bir
  tests/c/external/gcc_torture/src/20140828-1.c` wrote
  `build/agent_state/653_step3_pointer_base_plus_offset_producer_fact/prepared_dump.txt`.
- `build/c4cll --target riscv64-linux-gnu --dump-bir
  tests/c/external/gcc_torture/src/20140828-1.c` wrote
  `build/agent_state/653_step3_pointer_base_plus_offset_producer_fact/semantic_bir.txt`.

Focused `src/20140828-1.c` probe:
`tests/backend/cmake/run_rv64_gcc_torture_backend_object_case.cmake` still
fails at object compile with `unsupported_terminator_fragment` because the
current prepared dump lacks the real `%t6` preserved `source_selection`.
