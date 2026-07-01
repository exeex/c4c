Status: Active
Source Idea Path: ideas/open/444_frame_slot_call_argument_publication.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Repair Frame-Slot Argument Publication And Consumption

# Current Packet

## Just Finished

Completed `plan.md` Step 3 repair for frame-slot address call-argument
publication and RV64 object-route consumption.

Implementation rule:

- `PreparedCallArgumentValuePublicationFact` now carries optional
  `payload_frame_slot_id`, `payload_stack_offset_bytes`, `payload_size_bytes`,
  and `payload_align_bytes` fields.
- `populate_call_argument_value_publication_plans` fills those fields only
  when the selected store-local source has a unique same-instruction default
  frame-slot address materialization for the payload value, with non-volatile
  default destination access and in-frame slot bounds.
- RV64 object emission now consumes that payload route directly by materializing
  `payload_stack_offset_bytes` with `addi scratch, sp, offset`, storing it into
  the argument object slot, then materializing the argument object address into
  the ABI destination register.
- Existing explicit `LoadLocal` payload publication remains supported only
  when the prepared store-source record proves a matching `source_load_local`.
  Unknown producers without a coherent frame-slot payload route fail closed.

Files changed:

- `src/backend/prealloc/publication_plans.hpp`
- `src/backend/prealloc/publication_plans.cpp`
- `src/backend/prealloc/prepared_printer/select_chains.cpp`
- `src/backend/mir/riscv/codegen/object_emission.cpp`
- `tests/backend/mir/backend_riscv_object_emission_test.cpp`

Representative result:

- Fresh prepared BIR for `tests/c/external/gcc_torture/src/20080506-2.c`
  now prints `payload_frame_slot=#6 payload_stack_offset=0 payload_size=4
  payload_align=4` on the arg1 `call_arg_value_publication` fact.
- Fresh RV64 object disassembly no longer emits stale `mv t1,s1`;
  `sd t1,16(sp)` for the second pointer argument is fed by `mv t1,sp`
  before `addi a1,sp,16`.
- `clang_qemu_rc=0` and `c4c_qemu_rc=0`.

## Suggested Next

Execute Step 4 by adding focused ordinary-C coverage for address-of-local
pointer arguments with multiple pointer arguments and a nonzero argument
position that would fail if the second argument reused stale register state.

## Watchouts

- `missing_frame_slot_arg_publication=yes` remains a need marker; it was not
  cosmetically suppressed.
- Text-route `prepared_call_emit.cpp` was not changed in this packet. Its
  existing frame-slot-address behavior should be considered separately if Step
  4 coverage includes text-route proof.
- The unit-test fixture was updated to include explicit frame-slot payload
  authority and to keep the separate `LoadLocal` payload route covered.
- No expectations, allowlists, unsupported markers, runtime comparison, or
  pass/fail accounting were changed.

## Proof

- Hook state aligned with
  `scripts/plan_review_state.py set-step --step-id 3 --step-title 'Repair Frame-Slot Argument Publication And Consumption'`.
- `cmake --build build --target c4cll` passed.
- Fresh representative artifacts are under
  `build/agent_state/444_step3_repair_frame_slot_arg/`.
- Focused representative object route passed:
  `prepared_bir_rc=0`, `c4c_obj_rc=0`, `clang_build_rc=0`,
  `clang_qemu_rc=0`, `c4c_link_rc=0`, `c4c_qemu_rc=0`.
- `cmake --build build --target backend_riscv_object_emission_test` passed.
- `ctest --test-dir build --output-on-failure -R '^backend_riscv_object_emission$'`
  passed 1/1.
- `ctest --test-dir build -j --output-on-failure -R "^backend_"` passed
  330/330.
- `test_after.log` contains the combined proof output.
