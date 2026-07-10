Status: Active
Source Idea Path: ideas/open/653_stack_carried_pointer_source_publication_materialization.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Implement The Narrow Stack-Carried Pointer Rule

# Current Packet

## Just Finished

Step 3, `Implement The Narrow Stack-Carried Pointer Rule`: investigated the
real `tests/c/external/gcc_torture/src/20140828-1.c` `%t6` producer/hand-off
gap and stopped before implementation because the required producer fact is not
present in structured prepared input.

- Evidence written to
  `build/agent_state/653_step3_real_t6_source_selection_handoff/summary.md`.
- Prepared dump still shows `preserve value=%t6 value_id=19 route=stack_slot
  spill_slot=slot#16+stack8 ...` without `source_selection`.
- Semantic BIR for `main` compares `%t4` against `%t6`, but there is no
  defining `%t6 = %lv.a.0 + 2` instruction in the block.
- Prepared value homes record `%t6` value id `19` as `kind=stack_slot
  slot_id=16 offset=8`, not `PointerBasePlusOffset`.
- Branch stack-load authority proves only stack-slot freshness for `%t6`; it
  does not carry the semantic local-frame pointer source.
- A producer-side attachment would have to infer `&a[1]` from source spelling,
  stack offsets, or final comparison shape, so the slice was left incomplete
  instead of adding testcase-shaped logic.

## Suggested Next

Executor packet: publish a real semantic producer fact for the undefined
`main` `%t6` value in `src/20140828-1.c`, so prepared state can bind value id
`19` to `&a[1]` / `%lv.a.0 + 2` before call preservation tries to attach a
stack-carried pointer `source_selection`.

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
  `build/agent_state/653_step3_real_t6_source_selection_handoff/prepared_dump.txt`.
- `build/c4cll --target riscv64-linux-gnu --dump-bir
  tests/c/external/gcc_torture/src/20140828-1.c` wrote
  `build/agent_state/653_step3_real_t6_source_selection_handoff/semantic_bir.txt`.

Focused `src/20140828-1.c` probe:
`tests/backend/cmake/run_rv64_gcc_torture_backend_object_case.cmake` still
fails at object compile with `unsupported_terminator_fragment` because the
current prepared dump lacks the real `%t6` preserved `source_selection`.
