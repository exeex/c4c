Status: Active
Source Idea Path: ideas/open/539_rv64_object_scalar_fragment_helper_cleanup.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Prune Dead Scalar Wrappers

# Current Packet

## Just Finished

Step 3: caller-proofed and pruned the dead scalar-adjacent object-emission
helper bodies left after the Step 2 extraction.

Changed files:

- `src/backend/mir/riscv/codegen/object_emission.cpp`
- `test_after.log`
- `todo.md`

Removed after direct caller proof:

- Removed object-side `materializable_fpr_immediate_bits`; AST caller check
  reported no callers in `object_emission.cpp`, and `rg` showed the remaining
  live copy only in `prepared_scalar_emit.cpp` for scalar return emission.
- Removed object-side `prepared_stack_slot_home_absolute_offset_for_value`;
  AST caller check reported no callers in `object_emission.cpp`, and `rg`
  showed the remaining live copy only in `prepared_scalar_emit.cpp`.
- Removed object-side `prepared_stack_slot_home_offset_for_value`; AST caller
  check reported no callers in `object_emission.cpp`, and `rg` showed no
  remaining object-side references.

Retained after caller proof:

- Retained `append_fragment`/`append_rv64_fragment`; direct callers still cover
  select-edge source publication, move-bundle assembly, select dependency
  assembly, and call fragment assembly.
- Retained local prepared stack-slot offset, scalar size, and floating-type
  helpers because variadic, move-bundle, call, select-publication, object-data,
  formal-entry, and diagnostic paths still call them.
- Retained low-level store/load/move wrappers because direct callers still cover
  variadic helpers, call preservation, predecessor select publication,
  out-of-SSA moves, inline asm, cast-dependency materialization, and saved
  callee-register spill/restore paths.
- Retained scalar API declarations in `prepared_scalar_emit.hpp`; `rg` shows
  they are still shared by `prepared_function_emit`, call, global-memory, and
  local-memory emitters as well as the moved object scalar fragments.

## Suggested Next

Run the supervisor-selected review/commit path for the completed Step 3 prune
slice.

## Watchouts

- `prepared_scalar_emit.cpp` still owns live copies of the removed object-side
  helper logic; do not prune those copies without a separate caller-proofed
  packet.
- Select-edge publication, predecessor publication movement, call lowering,
  memory/object-data emission, function traversal, and dispatch remain parked
  in `object_emission.cpp` and keep their local helper boundaries.
- No expectations, unsupported markers, `plan.md`, source idea, or review
  artifacts were touched.
- Do not touch `review/global_address_helper_cleanup_review.md`.

## Proof

Passed; proof log is `test_after.log`.

```bash
bash -o pipefail -c "cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R 'backend_(riscv_object_emission|dump_riscv64_prepared_fused_compare|codegen_route_riscv64_prepared_fused_compare|obj_runtime_rv64_return_add|obj_runtime_rv64_return_add_sub_chain)'" > test_after.log 2>&1
```

Result: build succeeded; 7/7 selected tests passed.
