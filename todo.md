Status: Active
Source Idea Path: ideas/open/599_pointer_base_plus_offset_selected_authority.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Prove Fail-Closed Behavior

# Current Packet

## Just Finished

Completed Step 4 from `plan.md`: added focused fail-closed proof for the
migrated pointer-base-plus-offset store-source publication route.

Files changed:

- `tests/backend/mir/backend_store_source_publication_plan_test.cpp`
- `todo.md`
- `test_after.log`

Proof added:

- `backend_store_source_publication_plan_test` now asserts a good
  `PointerBasePlusOffset` store-local publication plan has selected
  `PointerBasePlusOffsetSource` freshness with exact
  `PreparedValueFreshnessUseKind::PointerBasePlusOffsetSource`,
  `PreparedValueFreshnessSourceKind::PointerBasePlusOffset`,
  `PreparedValueFreshnessProofKind::PointerBasePlusOffsetAuthority`, and
  `PreparedValueFreshnessSourceRank::PointerBasePlusOffset`, plus exact source
  home reference, result value id/name, base value, optional base symbol, byte
  delta, block label, and instruction index.
- The test proves `prepared_pointer_base_plus_offset_source_freshness_available(...)`
  accepts the good plan and rejects otherwise-available plans for missing/no
  candidate freshness, ambiguous freshness, stale/wrong program point,
  wrong-base, wrong-result, wrong-delta, wrong-use, wrong source kind, wrong
  proof kind, wrong rank, support-only evidence, target-shape-only evidence,
  and range-only evidence.
- The support-only case explicitly keeps
  `prepared_store_source_publication_available(...) == true` while omitting
  the publication program point, proving store-source support facts do not
  substitute for selected pointer-arithmetic authority.

## Suggested Next

Execute Step 5 from `plan.md`: closure inventory and follow-up decision for
the pointer-base-plus-offset selected authority route.

## Watchouts

- Keep pointer-value indirect memory-use freshness in idea 600.
- Do not treat home shape, byte delta, range/layout facts, stack/register
  placement, target offset encodability, target operand shape, diagnostics, or
  dumps as selected pointer-arithmetic authority.
- Existing producer-publication `source_freshness_*` fields are intentionally
  separate from
  `pointer_base_plus_offset_source_freshness_*`.
- The helper currently uses the existing generic freshness query statuses:
  `NoCandidate`, `InvalidCandidate`, `AmbiguousCandidate`, and `Selected`.
- Step 5 should keep closure notes scoped to the shared store-source route and
  the AArch64 representative consumer; broader target migration remains out of
  scope.
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
