Status: Active
Source Idea Path: ideas/open/391_rv64_variadic_prologue_save_area_publication.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Verify Closure Preconditions

# Current Packet

## Just Finished

Step 1 verified idea 391 closure preconditions using existing evidence only.
No build or tests were run in this packet.

Idea 392 is closed: the source now lives at
`ideas/closed/392_rv64_va_list_expression_call_argument_value_publication.md`
with `Status: Closed`. Its closure notes and Step 4 evidence show the later
`va_list` call-argument publication boundary is handled:
`build/agent_state/392_postrepair_step4_va-arg-13.run.log` records
`case_exit=0` and
`[PASS][rv64-gcc-torture-backend-obj] /workspaces/c4c/tests/c/external/gcc_torture/src/va-arg-13.c`.

Idea 391 remains the save-area publication owner. Its source note says the
save-area route was complete and that the remaining representative abort was
split to idea 392. Since idea 392 is closed and the representative is green,
idea 391 can proceed to close-gate regression comparison rather than another
implementation packet.

Current canonical logs cover the needed backend close-gate scope:
`test_before.log` reports `100% tests passed, 0 tests failed out of 326`, and
`test_after.log` reports the same backend bucket result. `test_after.log` also
contains the appended idea 392 representative pass, so a strict file-shape
normalization step may need to compare only the backend CTest portion or rerun
the backend-only after log if the close-gate tool requires symmetric root logs.
No missing canonical proof artifact was found.

Inspection evidence:
`build/agent_state/391_closure_step1_preconditions.log`.

## Suggested Next

Proceed to the close-gate regression comparison for idea 391 using the existing
backend `test_before.log` / `test_after.log` evidence, with the normalization
caveat above if the checker requires exactly symmetric root log contents.

## Watchouts

- `test_before.log` and `test_after.log` are local canonical proof artifacts,
  not tracked files; they are still present and readable in the workspace.
- The worktree was clean before this packet's `todo.md` update.
- Do not reopen idea 392 unless its closed acceptance evidence is invalidated.

## Proof

Proof command for this packet: inspect existing evidence only; no build or
tests run.

Evidence inspected:
`ideas/open/391_rv64_variadic_prologue_save_area_publication.md`,
`ideas/closed/392_rv64_va_list_expression_call_argument_value_publication.md`,
`test_before.log`,
`test_after.log`,
`build/agent_state/392_postrepair_step4_va-arg-13.run.log`, and
`build/agent_state/392_postrepair_step4_va-arg-13.case.log`.

Result: preconditions satisfied for close-gate comparison; no missing canonical
proof artifact identified.
