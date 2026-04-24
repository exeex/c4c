Status: Active
Source Idea Path: ideas/open/91_advanced_prepared_call_authority_and_grouped_width_allocation.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Publish Advanced Prepared Call Authority
Plan Review Counter: 2 / 6
# Current Packet

## Just Finished

Step 2 now extends the shared prepared call-plan plus prepared-printer proof to
the existing same-module variadic-byval authority surface in
`tests/backend/backend_prepare_liveness_test.cpp`. The lowered
`lowered_same_module_variadic_global_byval_metadata` fixture now proves that the
global byval payload publishes as a symbol-backed aggregate-address call
argument on the prepared call-plan and printer seams, and the refined
assertions now require a concrete ABI destination without hard-coding an x86
register spelling, avoiding target-local recovery or testcase-shaped
shortcuts.

## Suggested Next

Decide whether Step 2 still needs one more truthful publication packet for a
stack-backed before/after-call preservation edge or whether this same-module
variadic-byval extension is enough to move the runbook into Step 3 grouped
width `> 1` allocator work.

## Watchouts

- Keep this route target-independent; do not hide missing authority behind
  x86-local recovery.
- Do not reopen out-of-SSA follow-on work from idea 90 inside this runbook.
- Reject testcase-shaped shortcuts; the next packet must generalize across the
  owned advanced call or grouped-width failure family.
- Keep the prepared-printer checks pinned to shared authority facts such as
  wrapper kind, source encoding, symbol identity, and destination bank rather
  than broad clobber-set spelling that may change for unrelated reasons.
- Same-module variadic-byval currently publishes through register-backed source
  storage plus symbol-address call-plan authority; if that route changes, the
  next packet should repair the shared publication seam rather than weakening
  the proof.
- Require the existence of a concrete ABI destination on both the structured
  and printer seams, but do not pin acceptance to any target-local register
  spelling.

## Proof

`cmake --build --preset default`
`ctest --test-dir build -j --output-on-failure -R '^backend_'`
Result: passed on this packet.
Log: `test_after.log`
