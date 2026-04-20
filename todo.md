# Execution State

Status: Active
Source Idea Path: ideas/open/61_call_bundle_and_multi_function_prepared_module_consumption.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Canonicalize Prepared-Module Traversal
Plan Review Counter: 1 / 10
# Current Packet

## Just Finished

Step 2 packet extended the generic prepared guard/plain-cond consumption in
`src/backend/mir/x86/codegen/prepared_param_zero_render.cpp` and the x86 local
slot dispatcher so authoritative prepared branch conditions can rematerialize a
named `i32` compare source from prepared homes instead of falling back to the
raw carrier. With the narrowed slice, `backend_x86_handoff_boundary` is green
again and `c_testsuite_x86_backend_src_00040_c` advances past the old
guard-chain stop to the prepared call-bundle handoff.

## Suggested Next

Build the next packet at Step 3 around the new owned blocker now visible in
`00040`: consume authoritative `BeforeCall` / `AfterCall` bundles for the
direct extern call path without reopening local ABI inference or growing
another bounded x86 call lane.

## Watchouts

- The first attempt at this packet also bundled a new local-slot `and-zero`
  boundary fixture and wider direct-extern work; that mixed slice regressed the
  boundary test, so keep the next packet on one owned call-bundle seam and do
  not add more boundary expansion until the call-lane contract is stable.
- `00040` now fails with `x86 backend emitter requires the authoritative
  prepared call-bundle handoff through the canonical prepared-module handoff`,
  which is the right next blocker for idea 61. The next packet should consume
  that durable contract directly instead of re-deriving ABI setup/results
  inside x86.
- The plain-cond helper now depends on `PreparedValueLocationFunction` for
  authoritative compare-source materialization. If another case lacks a durable
  home for the compared value, treat that as a prepared-contract issue rather
  than reopening raw compare-carrier fallback.

## Proof

Ran the delegated proof command and preserved `/workspaces/c4c/test_after.log`:
`cmake --build --preset default && ctest --test-dir build --output-on-failure -R '^(backend_x86_handoff_boundary|c_testsuite_x86_backend_src_00040_c)$' | tee test_after.log`.
Result: `backend_x86_handoff_boundary` passed and
`c_testsuite_x86_backend_src_00040_c` now fails with
`x86 backend emitter requires the authoritative prepared call-bundle handoff through the canonical prepared-module handoff`.
