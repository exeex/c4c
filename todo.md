Status: Active
Source Idea Path: ideas/open/599_pointer_base_plus_offset_selected_authority.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Closure Inventory And Follow-Up Decision

# Current Packet

## Just Finished

Completed Step 5 from `plan.md`: recorded closure inventory and follow-up
decision for `ideas/open/599_pointer_base_plus_offset_selected_authority.md`.

Closure inventory:

1. Audited consumers:
   shared prealloc producer/support surfaces around
   `PreparedValueHomeKind::PointerBasePlusOffset`,
   `PreparedPointerBasePlusOffsetFact`,
   `as_pointer_base_plus_offset_fact(...)`,
   `prepared_contract_verifier.*`, `decoded_home_storage.*`,
   `prepared_lookups.cpp`, `publication_plans.*`, prepared printer surfaces,
   formal/storage/call planning, and object traversal classification. Narrow
   target consumers were audited in RV64 edge publication/scalar/frame/object
   emission paths, AArch64 operand/memory/call paths, and x86 module lowering
   rejection paths.
2. Selected authority dimensions:
   accepting a computed pointer use requires the exact computed result value
   id/name, source home kind `PointerBasePlusOffset`, base pointer value name,
   optional base symbol when present, byte delta, use kind, proof kind, source
   kind, rank, and store-source publication program point/reference. Home
   shape, byte delta, placement, range/encodability, target operand shape,
   diagnostics/dumps, and available store-source support facts remain
   insufficient.
3. Freshness vocabulary:
   no existing call, move, producer, edge, branch, select-carrier, alias, or
   pointer-value memory-use freshness kind was reused. The route added
   `PreparedValueFreshnessUseKind::PointerBasePlusOffsetSource`,
   `PreparedValueFreshnessSourceKind::PointerBasePlusOffset`,
   `PreparedValueFreshnessProofKind::PointerBasePlusOffsetAuthority`, and
   `PreparedValueFreshnessSourceRank::PointerBasePlusOffset`.
4. Representative consumer migrated:
   shared `PreparedStoreSourcePublicationPlan` /
   `plan_prepared_store_source_publication(...)` for a
   `PointerBasePlusOffset` source home, consumed target-side by AArch64
   `plan_pointer_base_plus_offset_store_local_publication(...)` /
   `lower_pointer_base_plus_offset_store_local_publication(...)`.
5. Fail-closed proof:
   `backend_store_source_publication_plan_test` proves the good plan selects
   `PointerBasePlusOffsetSource` authority and
   `prepared_pointer_base_plus_offset_source_freshness_available(...)` rejects
   missing/no-candidate, ambiguous, stale/wrong-program-point, wrong-base,
   wrong-result/wrong-value, wrong-delta, wrong-use, wrong source/proof/rank,
   support-only, range-only, and target-shape-only evidence while store-source
   support facts can remain otherwise available.
6. Separate follow-ups:
   pointer-value indirect memory-use freshness remains in idea 600; semantic
   GEP target consumption, relocation/materialization semantics, broad
   RV64/AArch64/x86 target migration, AArch64 generic operand/call migration,
   x86 lowering/rejection migration, and RV64 edge/scalar/frame/object
   migration remain separate initiatives.

## Suggested Next

Lifecycle close review by the plan owner if this closure inventory is
complete.

## Watchouts

- Closure should not broaden the completed route. The accepted implementation
  intentionally migrated one representative shared store-source route only.
- Keep `pointer_base_plus_offset_source_freshness_*` separate from existing
  producer-publication `source_freshness_*`.
- Follow-up ideas should be concrete and separate if opened: pointer-value
  memory-use freshness, semantic GEP target consumption, relocation or
  materialization semantics, or target-family migrations.

## Proof

Proof commands:

- Step 4 backend proof:
  `(cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_') > test_after.log 2>&1`
  passed 346/346.
- Hook full-suite baseline review after Step 4:
  regression guard PASS, 3375/3375 before and after; baseline accepted.
- Step 5 todo-only proof: `git diff --check` passed. No build/tests were run
  for this closure-only edit, and `test_after.log` was not updated.
