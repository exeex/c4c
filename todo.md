# Execution State

Status: Active
Source Idea Path: ideas/open/67_backend_trace_and_error_contract_for_x86_handoff.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Baseline The Current Debug Ladder Against `00204.c`
Plan Review Counter: 1 / 6
# Current Packet

## Just Finished

Step 1 baseline run completed for `tests/c/external/c-testsuite/src/00204.c`.
Captured the current debug ladder in `test_after.log` across
`--dump-bir`, `--dump-prepared-bir`, `--dump-mir`, and `--trace-mir`, and
confirmed the first concrete observability gap is x86 rejection clarity:
`match` reaches semantic BIR and prepared control-flow facts, but the x86
surface still ends at `final: no current top-level x86 lane matched` without a
plain-language rejection reason or a next-inspection pointer.

## Suggested Next

Implement Step 2.2 for the nearest x86 handoff boundary coverage plus
`00204.c`: make `--dump-mir` and `--trace-mir` report the final meaningful
rejection for `match` in plain language, distinguish ordinary lane miss from an
unsupported prepared shape or missing contract, and point the user toward the
next backend surface to inspect.

## Watchouts

- `--dump-bir` already answers the semantic-shape question for `match`, and
  `--dump-prepared-bir` at least confirms prepared CFG shape, so the next slice
  should not widen into semantic-lowering repair.
- The current x86 trace for `match` lists lane-by-lane rejects but never names
  which prepared contract blocked the final route or what to inspect next;
  favor one meaningful rejection over more raw trace volume.
- The supervisor-selected backend subset is currently red on
  `backend_prepare_liveness` before any idea 67 code changes, so treat that as
  pre-existing proof noise unless the next packet touches that area.

## Proof

Ran `cmake --build --preset default`, then
`ctest --test-dir build -j --output-on-failure -R "^backend_"`, then the
current `00204.c` ladder with `build/c4cll --dump-bir`,
`--dump-prepared-bir`, `--dump-mir`, and `--trace-mir` targeting
`x86_64-unknown-linux-gnu`. The backend subset is not fully green because
`backend_prepare_liveness` is already failing in the current tree; the large
case ladder still completed and is recorded in `test_after.log`.
