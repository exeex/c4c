Status: Active
Source Idea Path: ideas/open/598_select_carrier_alias_freshness_contract.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Migrate One Representative Consumer

# Current Packet

## Just Finished

Completed Step 3 from `plan.md`: migrated the representative select-carrier
alias source consumer to require selected source freshness.

Implemented boundary:

- Added narrow freshness vocabulary:
  `PreparedValueFreshnessUseKind::SelectCarrierAliasSource`,
  `PreparedValueFreshnessSourceKind::SelectCarrierAlias`,
  `PreparedValueFreshnessProofKind::SelectCarrierAliasAuthority`, and
  `PreparedValueFreshnessSourceRank::SelectCarrierAlias`.
- Extended the shared freshness lookup contract so only that use/source/proof
  and rank combination can select for the select-carrier alias source use.
  `MoveBundleSource` and `DirectEdgePublicationSource` remain separate owners.
- `plan_prepared_select_carrier_alias_authority(...)` now publishes and
  selects a source freshness authority only after the alias authority reaches
  `available`, with the queried source value id/name and scalar reference to
  the source producer block/inst.
- Added
  `prepared_select_carrier_alias_source_freshness_available(...)` in shared
  prealloc. It accepts only when the selected freshness authority matches the
  exact function, edge, destination, source value, source producer kind,
  source producer block/inst, proof, source kind, rank, and alias closure
  required by the Step 2 contract.
- The RV64 helper
  `prepared_select_edge_binary_source_has_carrier_alias_authority(...)` now
  delegates source acceptance to the shared-prealloc freshness check instead of
  accepting alias support facts directly. RV64 remains consume-only.

Files changed:

- `src/backend/prealloc/value_locations.hpp`
- `src/backend/prealloc/prepared_lookups.cpp`
- `src/backend/prealloc/publication_plans.hpp`
- `src/backend/prealloc/publication_plans.cpp`
- `src/backend/mir/riscv/codegen/prepared_edge_publication_emit.cpp`
- `todo.md`

## Suggested Next

Step 4: Prove Fail-Closed Behavior.

## Watchouts

- Do not claim progress through expectation rewrites, unsupported-marker
  edits, allowlist changes, diagnostics-only changes, or target-local shape
  checks.
- Step 4 should add focused tests or dump assertions that distinguish selected
  `SelectCarrierAliasSource` freshness from alias-only authority, destination
  legality, complete homes, target shape, structural join-transfer evidence,
  and wrong-use freshness candidates.
- Current code publishes select-carrier alias freshness from the existing
  available shared alias authority. Tests should still prove the consumer fails
  closed if the selected freshness field is missing, invalid, ambiguous, stale,
  wrong-value, or wrong-use.
- The target helper name still mentions carrier alias authority for call-site
  stability, but the semantic decision now lives in shared prealloc via
  `prepared_select_carrier_alias_source_freshness_available(...)`.
- Keep destination fan-in, predecessor-edge suppression, pointer/address
  follow-ups, target migration, and Prepared MIR view design separate.

## Proof

Ran `git diff --check`; passed.

Ran
`(cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_') > test_after.log 2>&1`;
passed on rerun with 346/346 backend tests passing. The first attempt was
interrupted by `cc1plus` being killed while compiling an AArch64 test object;
the exact same command was rerun and completed successfully. Proof log:
`test_after.log`.
