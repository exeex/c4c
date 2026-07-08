Status: Active
Source Idea Path: ideas/open/598_select_carrier_alias_freshness_contract.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Handoff And Broader Validation

# Current Packet

## Just Finished

Completed Step 5 from `plan.md`: recorded closure/handoff notes for the
select-carrier alias freshness contract route.

Closure answers:

1. Audited consumers/surfaces:
   `PreparedSelectCarrierAliasAuthority`,
   `PreparedSelectCarrierAliasAuthorityRecords`,
   `plan_prepared_select_carrier_alias_authority(...)`,
   `collect_prepared_select_carrier_alias_authorities(...)`,
   `collect_prepared_select_carrier_alias_authority_evidence(...)`,
   `populate_select_carrier_alias_identity(...)`,
   `append_select_carrier_alias_authorities(...)`,
   `classify_prepared_object_select_consumer(...)`,
   `diagnose_prepared_object_consumer(...)`, RV64 admission threading through
   `RiscvPreparedFunctionAdmissionResult`, RV64 helper
   `prepared_select_edge_binary_source_has_carrier_alias_authority(...)`, RV64
   carrier-alias select suppression through
   `prepared_select_is_authorized_carrier_alias(...)`, and the RV64 object
   emission call sites that pass carrier-alias authority records.
2. Ownership rule:
   the binary select-edge publication source is fresh for the representative
   select-carrier alias use only when a selected shared freshness authority
   matches the exact function, predecessor, successor, destination value/id/name,
   source value/id/name, binary source producer kind, source producer block/inst,
   proof, source kind, rank, and alias closure required by the selected
   publication/source use. Alias metadata, destination legality, suppression,
   complete homes, target shape, and structural join-transfer evidence are
   support facts only.
3. Freshness vocabulary:
   added `PreparedValueFreshnessUseKind::SelectCarrierAliasSource`,
   `PreparedValueFreshnessSourceKind::SelectCarrierAlias`,
   `PreparedValueFreshnessProofKind::SelectCarrierAliasAuthority`, and
   `PreparedValueFreshnessSourceRank::SelectCarrierAlias`. The route did not
   reuse `MoveBundleSource` or `DirectEdgePublicationSource`.
4. Migrated representative consumer:
   the source acceptance gate currently reached through RV64
   `prepared_select_edge_binary_source_has_carrier_alias_authority(...)` inside
   `prepared_select_edge_binary_source_has_authorized_consumers(...)` now
   delegates to shared-prealloc
   `prepared_select_carrier_alias_source_freshness_available(...)`.
5. Fail-closed cases:
   missing/no-candidate freshness, ambiguous freshness, stale or wrong program
   point/reference, wrong value, wrong use, wrong source kind, wrong proof,
   wrong rank, and alias-only authority all fail closed before source
   acceptance. Destination-only authority remains insufficient by contract; no
   destination-home, destination-register, stack-destination fan-in, or
   destination-bundle fact is used as select-carrier alias source freshness.
   Existing unavailable carrier-alias statuses such as missing source producer,
   mismatched alias, and non-carrier source use remain fail-closed.
6. Intentionally left out:
   destination fan-in authority, predecessor-edge consumed suppression,
   producer-publication repair outside this route, broad RV64/AArch64/x86/string
   target migration, move-bundle scheduler or parallel-copy redesign,
   pointer/address work, Prepared MIR view design, and dump-output expansion for
   selected carrier-alias freshness fields.
7. Proof surfaces:
   `check_select_carrier_alias_authority_contract()` now asserts selected
   `SelectCarrierAliasSource` freshness on accepted authority records and direct
   fail-closed behavior for alias-only, ambiguous, stale/wrong-reference,
   wrong-value, wrong-use, wrong-source, wrong-proof, and wrong-rank cases.
   Existing prepared dump assertions still prove carrier-alias evidence rows
   expose available and rejected alias authority status, candidate counts,
   aliases, and source-use closure, but the selected freshness fields are
   proven by direct contract tests rather than dump text.
8. Follow-up ideas:
   no required follow-up is needed to complete this source idea. Optional future
   ideas, if prioritized separately, are prepared dump output for selected
   carrier-alias freshness fields, destination fan-in authority,
   predecessor-edge consumed suppression, broader target consumer migration, and
   Prepared MIR view/interface cleanup.

Files changed in this packet:

- `todo.md`

## Suggested Next

Lifecycle close review by plan owner if these closure notes are complete.

## Watchouts

- This packet is todo-only; `test_after.log` was intentionally not updated.
- Closure should not claim broad destination fan-in, predecessor-edge consumed
  suppression, or target migration was solved.
- If plan owner wants dump visibility for selected carrier-alias freshness
  fields, open it as a separate observability follow-up rather than expanding
  this completed route.

## Proof

Prior Step 4 targeted proof:
`cmake --build --preset default --target backend_prepare_stack_layout_test && ctest --test-dir build -j --output-on-failure -R '^backend_prepare_stack_layout$'`;
passed.

Prior backend proof:
`(cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_') > test_after.log 2>&1`;
passed with 346/346 backend tests passing.

Hook full-suite baseline review after Step 4: regression guard PASS, 3375/3375
before and 3375/3375 after; baseline accepted.

For this todo-only packet, ran `git diff --check`; passed. Did not run
build/tests and did not update `test_after.log`.
