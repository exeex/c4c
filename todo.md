Status: Active
Source Idea Path: ideas/open/599_pointer_base_plus_offset_selected_authority.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Migrate One Representative Consumer

# Current Packet

## Just Finished

Completed Step 3 from `plan.md`: migrated the representative
pointer-base-plus-offset store-source publication route to require selected
pointer-arithmetic freshness before AArch64 materializes the computed pointer
source.

Files changed:

- `src/backend/prealloc/value_locations.hpp`
- `src/backend/prealloc/prepared_lookups.cpp`
- `src/backend/prealloc/publication_plans.hpp`
- `src/backend/prealloc/publication_plans.cpp`
- `src/backend/mir/aarch64/codegen/memory.cpp`
- `todo.md`

Semantic boundary implemented:

- Added distinct freshness vocabulary:
  `PreparedValueFreshnessUseKind::PointerBasePlusOffsetSource`,
  `PreparedValueFreshnessSourceKind::PointerBasePlusOffset`,
  `PreparedValueFreshnessProofKind::PointerBasePlusOffsetAuthority`, and
  `PreparedValueFreshnessSourceRank::PointerBasePlusOffset`.
- Added publication program-point reference fields to
  `PreparedStoreSourcePublicationPlan` / inputs and a separate
  `pointer_base_plus_offset_source_freshness_*` selected-authority channel so
  existing producer-publication source freshness remains unchanged.
- `plan_prepared_store_source_publication(...)` now publishes and selects the
  pointer-base-plus-offset source authority for store-local publication only
  when the source home is coherent, names the exact computed result value,
  carries the exact base value / optional base symbol / byte delta, and has the
  publication block label plus instruction index.
- `prepared_pointer_base_plus_offset_source_freshness_available(...)` now
  requires selected freshness to match value id/name, source home pointer,
  source/use/proof/rank vocabulary, base identity, optional base symbol, byte
  delta, and publication reference.
- AArch64 `lower_pointer_base_plus_offset_store_local_publication(...)` now
  consumes the shared helper before target materialization; it does not own
  target-local freshness semantics.

## Suggested Next

Execute Step 4 from `plan.md`: add focused proof that the migrated route uses
explicit selected `PointerBasePlusOffsetSource` authority and fails closed for
missing, ambiguous, stale/wrong-program-point, wrong-base, wrong-result,
wrong-delta, wrong-use, range-only, target-shape-only, and support-only
evidence.

## Watchouts

- Keep pointer-value indirect memory-use freshness in idea 600.
- Do not treat home shape, byte delta, range/layout facts, stack/register
  placement, target offset encodability, target operand shape, diagnostics, or
  dumps as selected pointer-arithmetic authority.
- Step 4 should test the new shared helper directly and the AArch64
  representative route. Existing producer-publication `source_freshness_*`
  fields are intentionally separate from
  `pointer_base_plus_offset_source_freshness_*`.
- The helper currently uses the existing generic freshness query statuses:
  `NoCandidate`, `InvalidCandidate`, `AmbiguousCandidate`, and `Selected`.
  Keep any new diagnostics narrow if Step 4 adds dump assertions.
- Target paths explicitly out of scope for this runbook: RV64 edge publication,
  scalar emit, frame/context helpers, and object-emission diagnostics; AArch64
  generic operand resolution, call lowering, and broad memory lowering beyond
  the representative store-local consumer; x86 module lowering/rejections;
  semantic GEP target consumption; relocation/materialization semantics.
- Do not reuse branch, edge-publication, move-bundle, select-carrier, alias,
  call-argument, producer-publication, or pointer-value memory-use freshness
  vocabulary for this route.

## Proof

Proof commands:

- `git diff --check` passed.
- `(cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_') > test_after.log 2>&1`
  passed: 346/346 backend tests.

Note: an earlier proof attempt hit an external compile resource failure while
building `backend_aarch64_instruction_dispatch_test.cpp`; rerunning the exact
delegated command incrementally passed and refreshed `test_after.log`.
