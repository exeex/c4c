# Current Packet

Status: Active
Source Idea Path: ideas/open/759_lir_typed_ref_enum_foundation.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Validate and hand off to 760

## Just Finished

- Plan Step 4 complete: `frontend_lir_call_type_ref_test` now proves enum-built
  i32, ptr, and void refs retain builtin id, kind, width where applicable, and
  rendered text. It mutates the i32 rendered text and proves id/width remain
  enum-authoritative; a dynamic array remains text-compatible with no builtin
  id.

## Suggested Next

- Step 5 validation and handoff: inspect the final diff, run the required
  focused LIR/frontend/backend validation, and preserve 760 as successor.

## Watchouts

- Do not remove runtime string construction in 759.
- Do not start the `[[deprecated]]` migration; that belongs to 760 after 759 is
  accepted and closed.
- Do not relax verifier rules or change HIR/BIR/backend semantics to make the
  enum foundation pass.
- Dynamic vector, array, struct, function, opaque, VRM, and arbitrary integer
  spellings remain supported text-backed inputs in this foundation slice.
- The mutable `str()` compatibility surface intentionally does not rewrite an
  enum-built ref's cached typed authority.

## Proof

- Passed: `cmake --build --preset default && ctest --test-dir build -j
  --output-on-failure -R '^frontend_lir_call_type_ref$' > test_after.log`.
- `test_after.log` contains the passing `frontend_lir_call_type_ref` subset.
