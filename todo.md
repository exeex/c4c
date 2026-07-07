Status: Active
Source Idea Path: ideas/open/580_rv64_scalar_compare_publication.md
Source Plan Path: plan.md
Current Step ID: Step 4
Current Step Title: Prove Representative Route Advancement

# Current Packet

## Just Finished

Re-executed `plan.md` Step 4 representative RV64 object-route proof after the
zero-immediate compare-publication follow-up.

`tests/c/external/gcc_torture/src/20080529-1.c` now advances past the previous
`unsupported_scalar_compare_publication` owner. The route exits `1` and now
stops at a later owner:
`unsupported_call_abi`, `function=main`, `instruction_index=0`,
`callee=test`, `result=i32 %t0`.

`tests/c/external/gcc_torture/src/loop-8.c` also stays past
`unsupported_scalar_compare_publication`. The route exits `1` and still stops
at the later pre-terminator move-bundle owner:
`unsupported_move_bundle_target_shape`,
`fragment_status=generic_move_bundle_materialization_failed`, `function=bar`,
`block_label=logic.rhs.end.3`, `instruction_index=0`.

Prepared dumps were rerun for both representatives and both returned `0`.

## Suggested Next

Proceed to supervisor review of whether Step 4 satisfies the route-advancement
gate, then choose either closure-readiness validation for this idea or a new
packet for the later `unsupported_call_abi`/move-bundle owners if those belong
inside the active source idea.

## Watchouts

- Neither representative route passes yet; both now fail on later non-compare
  owners.
- This packet did not touch source, tests, `plan.md`, or source idea files.
- The untracked `ideas/open/583_rv64_pointer_arithmetic_result_publication.md`
  remains unrelated and untouched.

## Proof

Commands/artifacts:

- Build freshness check: `cmake --build --preset default --target c4cll`
  returned `0` (`ninja: no work to do`).
- `build/agent_state/580_rv64_scalar_compare_publication/step4b/src_20080529-1.c/object-route.cmd`
  returned `1`; stdout/stderr/merged log are saved as `object-route.out`,
  `object-route.err`, and `object-route.log` in the same directory.
- `build/agent_state/580_rv64_scalar_compare_publication/step4b/src_20080529-1.c/dump-prepared-bir.cmd`
  returned `0`; dump artifacts are saved as `dump-prepared-bir.txt`,
  `dump-prepared-bir.err`, and `dump-prepared-bir.rc` in the same directory.
- `build/agent_state/580_rv64_scalar_compare_publication/step4b/src_loop-8.c/object-route.cmd`
  returned `1`; stdout/stderr/merged log are saved as `object-route.out`,
  `object-route.err`, and `object-route.log` in the same directory.
- `build/agent_state/580_rv64_scalar_compare_publication/step4b/src_loop-8.c/dump-prepared-bir.cmd`
  returned `0`; dump artifacts are saved as `dump-prepared-bir.txt`,
  `dump-prepared-bir.err`, and `dump-prepared-bir.rc` in the same directory.

No root `test_after.log` was produced because the delegated proof requested
per-case route artifacts under `step4b/`.
