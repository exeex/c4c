# 499 Step 3 Semantic GEP Local-Memory Producer

Implemented the first production BIR semantic GEP local-memory admission packet.

## Files Changed

- `src/backend/bir/bir.hpp`
- `src/backend/prealloc/publication_plans.cpp`
- `tests/backend/bir/backend_lir_to_bir_notes_test.cpp`
- `tests/backend/mir/backend_publication_plan_record_test.cpp`
- `todo.md`

## Implementation

- Added `bir::LocalArraySemanticGepRecord` and
  `bir::Function::local_array_semantic_geps`.
- Added `bir::evaluate_local_array_semantic_gep`, which packages semantic GEP
  admission only from an existing `LocalArrayLocalAddressProvenanceRecord`.
- Added the semantic GEP publication pass in `publication_plans.cpp`; the
  existing scalar-local-load publication entry point refreshes these records
  after `populate_local_array_local_address_provenances` has supplied the lower
  authority.

## Available Contract

`Available` requires a matching available local-address provenance record for a
local-object `AddressDerivation` path, available checker/range authority,
available source object and derivation records, available producer coordinate,
known nonzero element type/size, in-bounds layout/range, and no duplicate
provenance identity.

The implementation does not scan raw LIR/BIR shape, final homes, RV64/MIR
target behavior, testcase names, value names, or route-local slots to infer GEP
authority.

## Fail-Closed Coverage

Focused evaluator and publication coverage preserves fail-closed behavior for:

- missing provenance / lower authority
- non-available provenance
- global/static source object
- pointer/formal or integer-pointer provenance
- runtime/string/call boundary
- aggregate/member/flexible/alias boundary
- variadic boundary
- raw-shape-only and target-only evidence
- coordinate confusion and duplicate provenance identity
- non-address-derivation operation role

## Residual Route

This slice does not implement RV64/MIR lowering or global/static GEP admission.
The 6 global/static rows remain later-owner boundary work. The 53 non-local
boundary rows from Step 1 remain fail-closed or prerequisite-owned until
separate producer authority exists.

## Proof

```sh
cmake --build build -j2 > test_after.log 2>&1 && ctest --test-dir build -j2 --output-on-failure -R '^backend_' >> test_after.log 2>&1 && git diff --check >> test_after.log 2>&1
```

Result: passed. Output preserved in `test_after.log`.
