Status: Active
Source Idea Path: ideas/open/382_aarch64_dynamic_stack_vla_fixed_slot_addressing.md
Source Plan Path: plan.md
Current Step ID: Step 4
Current Step Title: Prove External Advancement and Guard Neighbors

# Current Packet

## Just Finished

Step 4 completed the proof and handoff for the accepted AArch64 dynamic-stack
fixed-slot repair. The delegated focused proof passes for the external
`00207` case, the dynamic-stack fixed-slot backend coverage, and the
`backend_aarch64_return_lowering` neighbor.

`c_testsuite_aarch64_backend_src_00207_c` now passes under the existing runner
and timeout policy instead of repeating `boom!` until timeout. The focused
backend route coverage confirms fixed-slot addressing uses the stable frame
anchor across dynamic stack movement, and the return-lowering neighbor remains
green.

No implementation, plan, source idea, expectation, unsupported-list, runner,
timeout-policy, or CTest-registration changes were made in this packet.

## Suggested Next

Proceed to Step 5 acceptance handoff, or route to lifecycle close review if the
supervisor considers the Step 4 proof and accepted regression evidence
sufficient for this idea.

## Watchouts

- The proof/handoff packet only refreshed `test_after.log` and aligned
  `todo.md`; it intentionally did not touch the accepted repair.
- Supervisor-side backend regression guard evidence is already accepted:
  before 148 passed / 0 failed / 148 total; after 149 passed / 0 failed /
  149 total.
- Supervisor-side full-suite baseline movement is already accepted:
  before 3363 passed / 17 failed / 3380 total; after 3365 passed / 16 failed /
  3381 total.
- Closed idea 381 remains untouched; `00200` shift/type-promotion scalability is
  still outside this owner.

## Proof

Ran exactly:

`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_aarch64_return_lowering|backend_codegen_route_aarch64_dynamic_stack_fixed_slot_uses_fp_anchor|c_testsuite_aarch64_backend_src_00207_c)$' > test_after.log 2>&1`

Result: build succeeded; all three selected CTests passed. `test_after.log` is
the canonical proof log for this packet.

Selected tests:
- `backend_aarch64_return_lowering`
- `backend_codegen_route_aarch64_dynamic_stack_fixed_slot_uses_fp_anchor`
- `c_testsuite_aarch64_backend_src_00207_c`

The refreshed `test_after.log` shows `00207` passing in 0.05 seconds; the
external case no longer has the repeated `boom!` timeout.

Already-landed supervisor backend regression guard passed with matching
commands:

`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_' > test_before.log 2>&1`

then, after restoring this slice:

`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_' > test_after.log 2>&1`

Guard result: before 148 passed / 0 failed / 148 total; after 149 passed /
0 failed / 149 total; no new failures.

Already-accepted full-suite baseline movement: before 3363 passed / 17 failed /
3380 total; after 3365 passed / 16 failed / 3381 total.
