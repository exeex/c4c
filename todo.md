Status: Active
Source Idea Path: ideas/open/844_lir_global_extern_initializer_family_facts.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Retire selected extern legacy mirrors only with parity

# Current Packet

## Just Finished

Completed repaired plan.md Step 4 for the selected extern aggregate return
target as a bounded no-code retirement conclusion. No safe deletion remains
inside the selected extern aggregate return slice: `return_type_str` is no
longer semantic authority for verifier/printer behavior when a valid structured
`return_type` carrier exists, but it still remains compatibility/output text
and fallback for nonaggregate runtime-text declarations and unselected extern
parameter/signature-store work.

## Suggested Next

Send the exhausted repaired runbook to plan-owner for semantic disposition of
idea 844: close as completed for the selected global and extern aggregate
return slices, repair the route for another in-scope extern parameter target,
or switch to a separately scoped successor if remaining durable intent should
continue elsewhere.

## Watchouts

Keep this repaired route inside idea 844 global/extern type facts. Do not absorb
initializer payload semantics, global policy identity, collector-only migration,
Raw-BIR receiver work, or non-type string routing. Extern parameter/signature
store facts remain a later target because `LirExternDecl` has no direct
parameter carrier today.

Do not widen Step 4 into extern parameter/signature-store migration,
collector/import-preparation, Raw-BIR receiver work, or nonaggregate runtime
text handling.

Baseline review note: hook candidate `test_baseline.new.log` at commit
`3323d25e2` regressed five varargs tests. The candidate was rejected with
`scripts/plan_review_state.py reject-baseline`; commit `17c5aaf88` narrowed the
global aggregate verifier gate, and a fresh full-suite proof passed 3038/3038.

## Proof

Step 4 proof command: full-suite `{ cmake --build --preset default && ctest --test-dir build -j --output-on-failure; } > test_after.log 2>&1`, passed 3038/3038.
