# Execution State

Status: Active
Source Idea Path: ideas/open/67_backend_trace_and_error_contract_for_x86_handoff.md
Source Plan Path: plan.md
Current Step ID: 3.1
Current Step Title: Isolate The Next Remaining Meaningful Rejection Family
Plan Review Counter: 2 / 6
# Current Packet

## Just Finished

Step 3.1 now upgrades the focused `fr_hfa11` x86 final rejection so the
remaining `00204.c` HFA proof path no longer mixes one structured
floating-aggregate helper report with one plain return-helper rejection: the
route-debug renderer now emits stable structured floating-aggregate
return-helper facts in both summary and trace, and the focused `--dump-mir` /
`--trace-mir` lane now surfaces `prepared floating-aggregate return-helper
facts: same-module global loads=1, scratch slot stores=0, scratch slot
reloads=0, sret floating stores=1`. The nearest reduced route-debug fixture
now locks exact summary/trace facts for the sret copyout helper, and
`tests/backend/CMakeLists.txt` requires the stable return-helper fact labels
alongside the existing call-helper labels on the grouped `00204.c` HFA CLI
lane without freezing the live helper counts.

## Suggested Next

If idea 67 still needs another packet, re-scan the remaining focused
`00204.c` rejection families for the next plain final rejection that still
lacks structured `final facts` or a lane-specific `next inspect` surface now
that both HFA helper and HFA return-helper lanes are covered.

## Watchouts

- The floating-aggregate return-helper facts intentionally count only
  BIR-visible copyout shape. Focused `fr_hfa11` currently reports `scratch
  slot stores=0` and `scratch slot reloads=0`, so do not infer hidden scratch
  staging from the preexisting detail text alone.
- The summary/trace contract remains two-part: keep the rejection line stable,
  then add a separate `final facts` line for the structured helper-family
  counts.
- The grouped `00204.c` CLI tests only require the floating-aggregate
  call-helper and return-helper fact labels, not the live `fr_hfa11` numbers.
  Preserve that flexibility unless the supervisor explicitly wants a stricter
  golden contract.

## Proof

Ran the delegated proof command and it passed:
`(cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_x86_route_debug|backend_cli_dump_mir_00204_hfa_rejection|backend_cli_trace_mir_00204_hfa_rejection)$' && build/c4cll --target x86_64-unknown-linux-gnu --dump-mir --mir-focus-function fr_hfa11 tests/c/external/c-testsuite/src/00204.c && build/c4cll --target x86_64-unknown-linux-gnu --trace-mir --mir-focus-function fr_hfa11 tests/c/external/c-testsuite/src/00204.c) > test_after.log 2>&1`
The resulting focused contract is:
`- final facts: prepared floating-aggregate return-helper facts: same-module global loads=1, scratch slot stores=0, scratch slot reloads=0, sret floating stores=1`
and the matching trace now carries the same structured final-facts line for
focused `fr_hfa11`. Proof log path: `test_after.log`.
