Status: Active
Source Idea Path: ideas/open/513_rv64_stack_to_stack_prepared_move_materialization.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Materialize Coherent Stack-To-Stack Moves

# Current Packet

## Just Finished

Completed Step 3 from `plan.md`: materialized the coherent RV64 stack-slot to
stack-slot prepared move family in
`src/backend/mir/riscv/codegen/object_emission.cpp`.

Implementation boundary:

- Added `fragment_for_prepared_stack_slot_to_stack_slot_move` under
  `fragment_for_prepared_move_bundle`.
- The helper accepts a per-move stack-to-stack entry inside a non-parallel-copy
  before-instruction bundle with `authority=none`, no parallel-copy source, no
  cycle-temp source, no immediate source, no explicit destination stack offset,
  destination storage `stack_slot`, contiguous width `1`, and reason
  `consumer_stack_to_stack`.
- Source and destination homes must both be prepared stack-slot homes with
  materializable slot id, offset, size, and alignment facts.
- Source and destination BIR types must both exist, be identical, and be a
  supported RV64 integer or pointer scalar width from
  `rv64_scalar_memory_size_for_type`.
- If storage-plan facts are present for either endpoint, they must describe a
  single-width GPR frame-slot value with slot id and stack offset. Explicit
  storage-plan `frame_slot bank=none`, FPR, vector, missing slot, missing
  offset, or multi-width facts remain fail-closed.
- If a stack-slot home carries a target register identity, the identity must be
  RV64 GPR/general. FPR, vector, or other incoherent identities remain
  fail-closed.
- Emission uses `rv64_unoccupied_temporary_gpr` and the existing
  `append_rv64_load_stack_offset_to_register` plus
  `append_rv64_store_register_to_stack_offset` helpers.

Focused coverage in
`tests/backend/mir/backend_riscv_object_emission_test.cpp` now proves a typed
same-scalar stack-slot copy emits `lw t1, 4(sp); sw t1, 8(sp)`, and reject
coverage keeps missing slot ids, size/home mismatches, unavailable scratch GPR,
incoherent source storage identity, explicit storage-plan `frame_slot
bank=none`, and reason/source-home mismatches fail-closed at the move-bundle
diagnostic. Coverage also proves a mixed non-parallel before-instruction bundle
can emit an already-supported register-to-stack move followed by a coherent
stack-to-stack move.

## Suggested Next

Execute Step 4 from `plan.md`: confirm and record focused materialization
coverage for accepted stack-to-stack moves and adjacent malformed/missing
authority reject cases before representative-row reclassification.

## Watchouts

This slice intentionally did not make `pr69447.c` acceptable: the traced first
failure is a `zext i16 to i64` shape with `bank=none` source storage, not a
same-type scalar copy. `20010518-1.c` and `pr27073.c` also need reclassification
because Step 2 found register-source first failures despite the
`consumer_stack_to_stack` reason string.

## Proof

Ran the delegated proof:

- `cmake --build build --target c4cll`
- `cmake --build build --target backend_riscv_object_emission_test`
- `ctest --test-dir build -j --output-on-failure -R 'backend_riscv_object_emission|stack_to_stack|move_bundle' | tee test_after.log`
- `ctest --test-dir build --output-on-failure -R '^backend_obj_runtime_rv64_frame_slot_pointer_arg_preserves_payload$'`

`test_after.log` contains the passing focused CTest run: 1/1 test passed.
The specific mixed call-boundary runtime regression also passed: 1/1 test
passed.
