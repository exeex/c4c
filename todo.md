Status: Active
Source Idea Path: ideas/open/381_aarch64_shift_promotion_codegen_scalability_timeout.md
Source Plan Path: plan.md
Current Step ID: Step 3
Current Step Title: Repair the Backend Scalability Owner

# Current Packet

## Just Finished

Step 3/4 external advancement repair is complete for the prepared AArch64
call-boundary scalability route.

- Kept the reviewer-requested cache-lifetime fix: the new indexes remain
  request-local/context-local, with no address-keyed `thread_local` pointer
  caches reintroduced.
- Added request-local prepared move-bundle and value-home indexes for AArch64
  lowering, and routed hot call-boundary/dispatch value-home lookups through
  those indexes with linear fallbacks for manually assembled test contexts.
- Repaired the remaining real `00200.c` hot path by using the prepared
  call-plan prior-preserved index for same-block/dominating prior probes instead
  of rescanning prior calls from each preserved value.
- Reduced remaining prealloc/lowering overhead with indexed storage-plan
  regalloc lookups, call-preservation candidate filtering, indexed regalloc
  call-move name lookups, vector-backed prior-preserved indexes, exact
  address-materialization lookup by instruction, and block instruction-stream
  reservation.
- The focused backend scalability route remains a static prepared
  call-boundary stress case and now passes inside the existing 5-second bound;
  the external `c_testsuite_aarch64_backend_src_00200_c` also advances past the
  asm-generation timeout and passes.

## Suggested Next

Return to the supervisor for review/commit handling. If review wants a follow-up
slice, keep it scoped to route-quality review of the focused test shape versus
the now-green external `00200.c` gate.

## Watchouts

- The repair is general and does not special-case `00200`, `lshift-type.c`, a
  test name, or the c-testsuite timeout policy.
- `rg` no longer finds `thread_local Cache` or `static thread_local` in the
  reviewed AArch64/backend paths; the remaining `thread_local` in this area is
  the pre-existing scoped preserve-effect publication flag, not an
  address-keyed cache.
- The focused test was kept as a static multi-call prepared call-boundary stress
  case; unrelated literal/debug payload and promotion-heavy argument expressions
  were removed so the 5-second bound measures the repaired backend route rather
  than frontend expression/literal processing.
- Do not work on parked idea 382 unless the supervisor switches lifecycle
  state.

## Proof

Delegated proof preserved in `test_after.log`:

First direct gate:

`timeout 20s ./build/c4cll --codegen asm --target aarch64-linux-gnu tests/c/external/c-testsuite/src/00200.c -o /tmp/c4c_00200_after_step3.s`

Outcome: completed with `DIRECT_RC=0`.

Final delegated proof:

`{ cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_|c_testsuite_aarch64_backend_src_00200_c$)' ; } > test_after.log 2>&1`

Outcome: build completed; CTest ran 149 matching tests. The focused
`backend_codegen_route_aarch64_prepared_call_boundary_scalability` test passed
in 1.14 seconds, `c_testsuite_aarch64_backend_src_00200_c` passed in 1.61
seconds, and all 149 tests passed.
