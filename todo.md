# Current Packet

Status: Active
Source Idea Path: ideas/open/759_lir_typed_ref_enum_foundation.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Add enum/id construction for LirTypeRef

## Just Finished

- Plan Step 2 complete: added `LirBuiltinType` for void, pointer, i1/i8/i16/
  i32/i64/i128, and half/float/double/fp128/x86_fp80. `LirTypeRef` now
  constructs directly from that enum and exposes `builtin_type()`; enum
  construction derives rendered text, kind, and integer width from enum state
  without reparsing text. Existing text constructors and dynamic spellings are
  retained; recognized compatible text also records the builtin id.

## Suggested Next

- Step 3 implementation: normalize the existing enum-first construction and
  typed query pattern for `LirBinaryOpcodeRef` and `LirCmpPredicateRef`.

## Watchouts

- Do not remove runtime string construction in 759.
- Do not start the `[[deprecated]]` migration; that belongs to 760 after 759 is
  accepted and closed.
- Do not relax verifier rules or change HIR/BIR/backend semantics to make the
  enum foundation pass.
- Dynamic vector, array, struct, function, opaque, VRM, and arbitrary integer
  spellings remain supported text-backed inputs in this foundation slice.

## Proof

- Passed: `cmake --build --preset default && ctest --test-dir build -j
  --output-on-failure -R '^frontend_lir_call_type_ref$' > test_after.log`.
- `test_after.log` contains the passing `frontend_lir_call_type_ref` subset.
