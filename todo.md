# Execution State

Status: Active
Source Idea Path: ideas/open/67_backend_trace_and_error_contract_for_x86_handoff.md
Source Plan Path: plan.md
Current Step ID: 3.1
Current Step Title: Isolate The Next Remaining Meaningful Rejection Family
Plan Review Counter: 1 / 6
# Current Packet

## Just Finished

Step 3.1 now upgrades the focused `fa_hfa11` x86 final rejection so `00204.c`
debugging no longer stops at a plain floating-aggregate call-helper rejection:
the route-debug renderer now emits stable structured floating-aggregate helper
facts in both summary and trace, and the focused `--dump-mir` /
`--trace-mir` lane now surfaces `prepared floating-aggregate helper facts:
aggregate pointer params=1, byval aggregate params=1, floating lane loads=0,
floating widening casts=1, direct variadic extern calls=1, floating variadic
call args=1`. The nearest reduced route-debug fixtures now lock exact
summary/trace facts for both the byval helper and pointer-wrapper variants,
and `tests/backend/CMakeLists.txt` requires the stable fact labels on the
grouped `00204.c` HFA CLI lane without freezing the live helper counts.

## Suggested Next

If idea 67 still needs another packet, stay on the neighboring focused
`00204.c` floating-aggregate family and upgrade the remaining
`fr_hfa11`/return-helper lane so the grouped HFA rejection no longer mixes one
structured call-helper report with one plain return-helper rejection.

## Watchouts

- The floating-aggregate helper facts intentionally count BIR-visible helper
  shape, not source declarations: `fa_hfa11` currently reports `floating lane
  loads=0` because the lowering already copied the float through locals before
  the focused helper reaches the direct variadic extern call.
- The summary/trace contract remains two-part: keep the rejection line stable,
  then add a separate `final facts` line for the structured helper-family
  counts.
- The grouped `00204.c` CLI tests only require the floating-aggregate fact
  labels, not the live `fa_hfa11` numbers. Preserve that flexibility unless
  the supervisor explicitly wants a stricter golden contract.

## Proof

Ran the delegated proof command and it passed:
`(cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_x86_route_debug|backend_cli_dump_mir_00204_hfa_rejection|backend_cli_trace_mir_00204_hfa_rejection)$' && build/c4cll --target x86_64-unknown-linux-gnu --dump-mir --mir-focus-function fa_hfa11 tests/c/external/c-testsuite/src/00204.c && build/c4cll --target x86_64-unknown-linux-gnu --trace-mir --mir-focus-function fa_hfa11 tests/c/external/c-testsuite/src/00204.c) > test_after.log 2>&1`
The resulting focused contract is:
`- final facts: prepared floating-aggregate helper facts: aggregate pointer params=1, byval aggregate params=1, floating lane loads=0, floating widening casts=1, direct variadic extern calls=1, floating variadic call args=1`
and the matching trace now carries the same structured final-facts line for
focused `fa_hfa11`. Proof log path: `test_after.log`.
