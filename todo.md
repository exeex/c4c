Status: Active
Source Idea Path: ideas/open/539_rv64_object_scalar_fragment_helper_cleanup.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Extract Scalar Fragment Helpers

# Current Packet

## Just Finished

Step 2: extracted the mapped RV64 scalar object fragment helpers into
`prepared_scalar_emit.*` without behavior changes.

Changed files:

- `src/backend/mir/riscv/codegen/object_emission.cpp`
- `src/backend/mir/riscv/codegen/prepared_scalar_emit.cpp`
- `src/backend/mir/riscv/codegen/prepared_scalar_emit.hpp`
- `test_after.log`
- `todo.md`

Completed extraction:

- Moved `append_rv64_move_value_to_register` into `prepared_scalar_emit.*` as
  the shared scalar prerequisite for moved helpers and still-parked object-side
  callers.
- Moved scalar binary, cast, compare branch, compare/trunc publication, and
  scalar return fragment construction into `prepared_scalar_emit.cpp`.
- Added scalar header declarations for the moved `RiscvEncodedFragment` helpers
  and moved the before-return already-loaded key type into that header so
  `object_emission.cpp` can keep owning before-return bundle discovery.
- Kept branch fixup construction, compare predicate normalization, direct
  global pointer return authority, FPR immediate return handling, and
  epilogue/ret emission behavior intact.

## Suggested Next

Run the supervisor-selected review/commit path for the completed Step 2 slice,
or delegate the next packet to caller-proof and trim any now-dead local wrapper
or declaration boundary left by the extraction.

## Watchouts

- Select-edge publication, predecessor publication movement, broad dispatch,
  `fragment_for_prepared_instruction`, and `fragment_for_prepared_terminator`
  remain parked in `object_emission.cpp` and now call the moved scalar APIs.
- Ordinary select lowering and pointer-specific fused branch helpers remain
  parked in `object_emission.cpp`; only their shared scalar move prerequisite is
  now imported from `prepared_scalar_emit.hpp`.
- Compare normalization, branch-label fixups, diagnostics, emitted bytes,
  tests, expectations, and unsupported markers were not intentionally changed.
- Do not touch `review/global_address_helper_cleanup_review.md`.

## Proof

Passed; proof log is `test_after.log`.

```bash
bash -o pipefail -c "cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R 'backend_(riscv_object_emission|dump_riscv64_prepared_fused_compare|codegen_route_riscv64_prepared_fused_compare|obj_runtime_rv64_return_add|obj_runtime_rv64_return_add_sub_chain)'" > test_after.log 2>&1
```

Result: build succeeded; 7/7 selected tests passed.
