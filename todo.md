Status: Active
Source Idea Path: ideas/open/193_hir_constructor_member_owner_structured_lookup_closure.md
Source Plan Path: plan.md
Current Step ID: Step 3
Current Step Title: Prove Stale Rendered Owner Fallback Rejection

# Current Packet

## Just Finished

Step 3 - Prove Stale Rendered Owner Fallback Rejection completed for direct
struct constructor owner lookup.

`frontend_hir_lookup_tests` now has focused direct-constructor lookup-unit
coverage for a qualified structured owner-key hit and for a complete structured
owner miss with a stale rendered constructor/struct collision present. The hit
case proves the direct constructor route calls the structured owner constructor;
the miss case proves it returns no expression and appends no constructor
lowering artifacts instead of using the stale rendered fallback.

## Suggested Next

Run the supervisor review/acceptance path for this Step 3 proof slice, then
choose the next narrow owner-lookup surface only if the active idea still needs
another metadata-rich route after direct constructors.

## Watchouts

This slice is test-only for the direct constructor route; it did not change
`object.cpp`. The success fixture uses qualified owner metadata and stale
rendered spelling so the proof does not rely on simple unqualified constructor
names.

## Proof

`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^frontend_hir_lookup_tests$' > test_after.log 2>&1`

Passed. `test_after.log` is the canonical proof log for this packet.
