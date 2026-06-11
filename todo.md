Status: Active
Source Idea Path: ideas/open/205_route6_call_use_source_adapter.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Prove Adapter Completeness And Route Quality

# Current Packet

## Just Finished

Step 4 completed as a proof-only packet. Existing focused coverage is enough
for the selected AArch64 scalar call argument source-producer adapter; no
additional tests were needed.

`scalar_call_argument_source_producer_reads_bir_materialization()` proves the
adapter accepts Route 6 only when the BIR call argument source agrees with the
prepared source id/name/value and same-block producer materialization is
available. It also proves absent Route 6 facts, source-id mismatch, duplicate
Route 6 facts, ABI-bound source-selection facts, and nonmaterializable producer
facts fall back to prepared producer lookup when present and reject route-only
materialization when no prepared producer lookup exists.

`verify_bir_call_argument_source_routing()` and
`verify_bir_call_argument_source_producer_materialization_lookup()` cover the
shared Route 6 record/index side: source-id agreement, missing/out-of-range
arguments, wrong calls, duplicate relationships, ABI-bound exclusion,
missing/future producers, load-local producers, binary producers, and
nonmaterializable producer status.

Unchanged-output coverage remains in the backend subset: the AArch64 selected
boundary still prints only the expected scalar producer materialization before
the call, direct-global call-argument dependency/fallback tests keep prepared
call-carrier behavior stable, and the x86 consumed-plan Route 6 helper tests
prove x86 Route 6 source reads fail closed while preserving prepared call
argument selection. This slice did not touch output expectations, unsupported
contracts, result lanes, direct-global dependency routing, whole call-plan
helpers, or ABI/layout/policy authority.

## Suggested Next

Ask the supervisor to decide whether to accept the completed Route 6
call-argument source-producer adapter slice, request reviewer scrutiny, or send
the lifecycle to plan-owner for closure.

## Watchouts

- Route 6 remains semantic identity only for this selected scalar call
  argument source-producer role; prepared call plans remain authoritative for
  ABI placement, wrappers, clobbers, outgoing stack sizing, byval lanes,
  variadic FPR counts, helper protocols, homes, move bundles, aggregate
  transport, publication policy, final call records, and emitted output.
- Remaining Route 6 boundaries, including result lanes, direct-global
  dependency routing, and whole call-plan helper groups, are non-goals for this
  runbook and should be handled as separate lifecycle work if needed.

## Proof

`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_' > test_after.log`

Passed: 180 backend tests.

Proof log: `test_after.log`.
