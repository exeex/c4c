Status: Active
Source Idea Path: ideas/open/640_mixed_local_global_publication_authority.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Publish One Shared Prepared Authority

# Current Packet

## Just Finished

Step 2 added the shared scalar frame-slot local-memory publication/consumer
repair by re-anchoring prepared memory-access position lookups after prepared
BIR transformations. The repair accepts only explicit prepared frame-slot
base-plus-offset facts that resolve back to the same `local_slot` stack object,
integer scalar type, block, slot, and stored/result value. Pointer locals,
global-symbol accesses, aggregate homes, volatile/TLS accesses, mismatched
slots, missing frame slots, and duplicate/ambiguous remaps remain fail-closed.

Focused coverage in `backend_prepare_stack_layout_test` simulates the
`src/pr70005.c`/`src/pr58431.c` shape where prepared scalar local access facts
carry stale instruction positions after inserted/prepared BIR reshaping. The
positive store/load remap is accepted; a pointer local frame-slot access in the
same fixture is not remapped through this scalar rule.

Supplemental row probe result: `src/pr58431.c` now passes the RV64 object
backend probe. `src/pr70005.c` advances past scalar frame-slot local memory to
`unsupported_prepared_move_bundle_classification` for an ambiguous
non-parallel multi-source stack destination in `fn1` at `logic.end.73`.
`src/pr57861.c`, `src/pr68185.c`, and `src/pr68321.c` remain fail-closed under
the residual local/pointer/order families and were not widened in this packet.

## Suggested Next

Delegate Step 3 to consume the next explicit authority family exposed by the
advanced rows, with `src/pr70005.c` as the cleanest next probe: the current
first owner is a move-bundle fan-in stack-destination publication authority,
not scalar local-memory addressing. Keep `src/pr68321.c` as a separate
publication-ordering/local-aggregate owner unless the supervisor deliberately
splits it.

## Watchouts

Do not generalize this repair to pointer-typed local slots or direct
global-symbol local memory. `src/pr58431.c` passes as the proving row, but
`src/pr57861.c` still has a pointer-to-global local path and `src/pr68185.c` /
`src/pr68321.c` still carry edge-store/local-array publication-ordering
evidence. The `src/pr70005.c` next owner is move-bundle publication, not more
frame-slot local-memory lookup repair.

## Proof

Ran the supervisor-selected proof command:

`cmake --build --preset default --target backend_prepare_stack_layout_test -j1 && ctest --test-dir build -j --output-on-failure -R '^backend_prepare_stack_layout$' > test_after.log 2>&1`

Result: passed. Canonical proof log: `test_after.log`.

Supplemental row probe:

`cmake --build --preset default && ALLOWLIST=build/agent_state/640_step1_mixed_publication.allowlist BUILD_DIR=build scripts/check_progress_rv64_gcc_c_torture_backend.sh > build/agent_state/640_step2_mixed_publication.log 2>&1`

Result: exited nonzero with `total=5 passed=1 failed=4`; `src/pr58431.c`
passed, proving one shared scalar frame-slot local-memory row advanced. Probe
log: `build/agent_state/640_step2_mixed_publication.log`.
