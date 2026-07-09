Status: Active
Source Idea Path: ideas/open/635_prepared_branch_stack_clobber_safety_authority.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Publish Call-Preservation-Aware Clobber-Safety Authority

# Current Packet

## Just Finished

Step 5 published prepared producer-side call-preservation-aware branch
stack-source clobber safety. Intervening calls now remain fail-closed unless
the prepared call plan explicitly preserves the selected source value in the
same stack slot, and intervening move bundles without direct stack offsets can
only prove non-clobber when their destination value home resolves to a known
different stack slot.

Focused prepared-layer coverage was added for preserved calls, calls without
preservation, mismatched preserved slots, stale preserved source identity,
explicit different-slot moves, and explicit same-slot moves.

The delegated backend probe moved the five Step 5 rows
`src/loop-2e.c`, `src/pr39100.c`, `src/20140828-1.c`, `src/20080519-1.c`,
and `src/20050125-1.c` past `missing_stack_clobber_safety`; they now stop at
`unsupported_terminator_fragment`. `src/20001017-1.c` remains
`unsupported_call_abi`, and `src/20000314-3.c` remains
`unsupported_terminator_fragment`. A refreshed trace for the anchor row is in
`build/agent_state/635_step5_loop-2e_prepared_dump.txt`.

## Suggested Next

Execute plan Step 6. Reclassify the seven representative rows after Step 5:
five rows have left clobber-safety authority and now point at terminator
lowering, one remains ABI, and one was already terminator lowering. Decide
whether idea `635` is close-ready or should split residual terminator/ABI work
to the appropriate initiatives.

## Watchouts

The move-bundle refinement intentionally accepts only explicit destination
value-home evidence for a different stack slot when the move itself lacks a
destination offset; unknown destinations and same-slot destinations remain
fail-closed. No RV64 target-local inference, final-shape inference,
expectation rewrite, unsupported marker, or allowlist change was made.

## Proof

Canonical proof:
`cmake --build --preset default --target backend_prepare_stack_layout_test -j1 && ctest --test-dir build -j --output-on-failure -R '^backend_prepare_stack_layout$' > test_after.log 2>&1`

Result: passed. Proof log: `test_after.log`.

Supplemental probe:
`cmake --build --preset default && ALLOWLIST=build/agent_state/635_step1_branch_clobber_safety.allowlist BUILD_DIR=build scripts/check_progress_rv64_gcc_c_torture_backend.sh > build/agent_state/635_step5_branch_clobber_safety.log 2>&1`

Result: failed with `total=7 passed=0 failed=7`, but the five Step 5 rows now
fail at `unsupported_terminator_fragment` instead of
`missing_stack_clobber_safety`. Supplemental log:
`build/agent_state/635_step5_branch_clobber_safety.log`.
