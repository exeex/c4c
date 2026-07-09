Status: Active
Source Idea Path: ideas/open/648_rv64_call_arg_frame_slot_address_materialization.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Refresh Post-656 Representative Call Evidence

# Current Packet

## Just Finished

Lifecycle reactivated idea 648 after closing idea 656. The earlier
`unsupported_local_memory_access` blocker no longer prevents the representative
`src/20000722-1.c` object route from emitting an object.

## Suggested Next

Execute Step 1 by refreshing post-656 prepared call evidence and RV64
asm/object diagnostics for `src/20000722-1.c`. Confirm whether the renewed
stale `mv a0, s1` argument setup is caused by RV64 consumption of
`arg.source_selection=local_frame_address_materialization` or by a distinct
owner that should be split.

## Watchouts

- Do not rely on pre-656 `mv a0, s2` evidence; refresh the representative row.
- Do not reopen idea 656 local-memory policy or string-label pointer admission.
- Do not infer frame-slot address materialization from source spelling, stack
  offsets, final assembly, testcase identity, or diagnostic text.
- Do not special-case `src/20000722-1.c`, `%lv._clit_`, `foo`, `bar`, `s1`,
  `s2`, or `a0`.
- Do not edit expectations, unsupported markers, allowlists, timeouts,
  runtime-comparison policy, or pass/fail accounting.

## Proof

Lifecycle-only reactivation. Close gate for idea 656 used the focused
local-memory label-pointer subset in `test_before.log` and `test_after.log`;
the non-decreasing regression guard passed with 4/4 tests green before and
after. A post-656 reprobe also confirmed the representative object route emits
an object and a fresh asm snapshot contains `mv a0, s1`.
