Status: Active
Source Idea Path: ideas/open/404_prepared_wide_rematerialized_immediate_admission.md
Source Plan Path: plan.md
Current Step ID: Step 1
Current Step Title: Classify Wide Immediate Producer Facts

# Current Packet

## Just Finished

Activated 404 after 411 closed. No implementation work has been performed in
this lifecycle packet.

## Suggested Next

Execute Step 1: classify the current `src/int-compare.c` wide rematerialized
immediate producer facts, identify the first bad prepared handoff, and record
the exact proof command for the first implementation packet.

## Watchouts

- Keep the repair at the BIR/prepared producer boundary unless proof shows a
  different owner.
- Do not special-case `int-compare.c`, `INT_MIN`, exact immediate spelling, or
  one testcase-shaped instruction sequence.
- Do not move missing constant-expression semantics into RV64 object emission.
- Route unrelated later residuals to separate owners instead of broadening
  404.

## Proof

Lifecycle activation only. No validation was run.
