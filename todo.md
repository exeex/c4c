# Current Packet

Status: Active
Source Idea Path: ideas/open/48_aarch64_dispatch_publication_prepared_authority_repair.md
Source Plan Path: plan.md
Current Step ID: Step 6
Current Step Title: Consume store-source publication planning for fixed-formal stores

## Just Finished

Step 6 routed fixed-formal store-local publication classification through
store-source publication planning instead of local BIR parameter identity
checks.

`publication_plans` now exposes
`plan_prepared_fixed_formal_store_source_publication`, a narrow wrapper around
`PreparedStoreSourcePublicationInputs`,
`plan_prepared_store_source_publication`, and prepared formal-publication
authority. It preserves the store-source plan, marks fixed-formal source
availability only when the prepared formal publication matches the planned
source home/value/type, rejects byval formals, and fails closed on ambiguity or
missing authority.

`lower_fixed_formal_store_local_publication` now builds a
`PreparedStoreSourcePublicationInputs` record from the prepared memory access,
source home, destination frame slot, and destination stack object. It requires
the shared fixed-formal store-source plan before materializing the local-slot
store, and computes the target frame address from planned destination facts
instead of using the removed `store_local_value_is_fixed_formal` BIR-param scan.

The store-source publication plan test covers the shared fixed-formal wrapper
and byval rejection. The AArch64 instruction dispatch test still covers the
fixed-formal local-slot route and now verifies fail-closed behavior when the BIR
parameter is still present but prepared formal/source-home authority is absent.

## Suggested Next

Ask the supervisor to decide whether idea 48 needs close-scope consolidation or
another targeted packet. If continuing within this idea, keep the next packet
bounded to a mapped publication-authority gap and use the existing prepared
planning/query seams.

## Watchouts

Keep the source idea stable during routine execution. Do not combine this
publication repair with the open comparison, memory, ALU, calls, or other
AArch64 follow-up scopes. Reject value-home spelling scans, same-block producer
scans, raw move-bundle scans, lane-name matching, and expectation downgrades as
progress.

Fixed-formal store publication now depends on prepared store-source and
formal-publication authority. Do not reintroduce local BIR parameter scans,
prepared-name/type matching, object-name recovery, lane suffix recovery, or
expectation downgrades.

Step 2 intentionally did not change `value_publication_may_read_register_index`;
the active plan treats that as register-hazard behavior unless it starts
reselecting semantic source or home facts.

Step 3 intentionally leaves target register spelling and source emission local
after prepared source-producer authority has authorized the fused-compare
producer-materialization route. Do not expand this packet into comparison,
ALU, memory, calls, or edge-copy emission refactors.

Step 5's frame-address query intentionally requires indexed prepared
address-materialization authority. A stack object name, rendered value spelling,
or aggregate lane suffix is not enough to authorize local address publication.

Step 6 used the existing `backend_store_source_publication_plan` target, whose
source file is `tests/backend/mir/backend_store_source_publication_plan_test.cpp`
in this tree.

## Proof

Ran:

`cmake --build --preset default && ctest --test-dir build -R '^(backend_aarch64_instruction_dispatch|backend_store_source_publication_plan|backend_prepared_lookup_helper)$' --output-on-failure | tee test_after.log`

Result: passed, 3/3 focused tests green. `test_after.log` now contains the
Step 6 proof.
