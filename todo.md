# Current Packet

Status: Active
Source Idea Path: ideas/open/803_lir_aggregate_ssa_producer_authority_publication.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Publish checked authority for selected aggregate producers

## Just Finished

- Step 2 repaired selected aggregate local-load and terminal-insertvalue
  authority handoffs and added compact mutation coverage for both forms:
  missing result authority, foreign/unknown ID, stale display, unselected
  producer, and type-incoherent aggregate receipt. The exact focused command
  passed (8/8) without changing canonical logs.

## Suggested Next

- Supervisor acceptance of completed Step 2, or the next in-scope plan packet.

## Watchouts

- Do not use display text or raw fallback, broaden to generic operand/
  expression provenance, add extractvalue index/layout/result rules, or touch
  Raw-BIR. Existing `modeled_scalar_result_type` is insufficient for
  `LirInsertValueOp`; Step 2 needs a narrowly named aggregate producer-type
  selection rather than treating all instruction results as aggregates.
  Do not change `LirAllocaOp.local_object_authority` or its raw-pointee
  equality rule. Preserve 754 Step 2 for the exact post-handoff return.

## Proof

- Passed fresh `cmake --build --preset default`, then the exact required CTest
  command with `-j --output-on-failure` and the specified regex: 8/8 passed.
  Canonical `test_before.log` and `test_after.log` were not modified.
