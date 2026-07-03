Status: Active
Source Idea Path: ideas/open/549_rv64_runtime_and_no_diagnostic_triage.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Split Durable Follow-Up Ideas

# Current Packet

## Just Finished

Step 3 split durable follow-up ideas from the Step 2 first-owner evidence
without closing the active 549 triage runbook.

Created follow-up ideas:

- `ideas/open/569_prepared_move_bundle_ambiguous_multi_source_stack_destination.md`
  for the high-confidence prepared move-bundle classifier boundary reproduced
  by `src/20001026-1.c`.
- `ideas/open/570_rv64_unsupported_instruction_fragment_owner_diagnostics.md`
  as an evidence-enabling diagnostic follow-up for the nine medium-confidence
  RV64 object-route rows that currently fail behind generic
  `unsupported_instruction_fragment`.

Unsplit / retained as evidence gaps in this active triage state:

- `src/20000910-1.c`: retained as F128 quarantine with caveat. Step 2 found
  F128 declarations/sections from `stdlib.h`, while the local source body is
  ordinary integer/pointer code; this is not durable proof for an ordinary-C
  or direct F128-operation repair idea.
- `src/20030307-1.c`: retained as a low-confidence evidence gap. BIR,
  prepared BIR, and MIR dumps succeed, but the object route reports only the
  generic `unsupported_instruction_fragment` diagnostic.

The nine generic RV64 object-route rows were intentionally not split into
capability-repair ideas. The durable split is diagnostic/instrumentation only
because Step 2 proved they reach RV64 object lowering but did not identify the
unsupported instruction, opcode, prepared authority boundary, or exact owner.

Source evidence used:

- `build/agent_state/549_step2_first_owner_classification/classification.md`
- `build/agent_state/549_step2_first_owner_classification/classification.tsv`
- `build/agent_state/549_step2_first_owner_classification/stage_matrix.tsv`

## Suggested Next

Execute Step 4: consolidate the 549 triage evidence for review. The summary
should name the two created follow-up ideas, the retained F128/evidence-gap
rows, and recommend whether this active runbook should close, run another
evidence pass, or switch to one of the created follow-up ideas.

## Watchouts

- Do not implement fixes during the triage packet.
- Do not weaken runtime comparison, expected outputs, unsupported markers, or pass/fail accounting.
- Route primary-F128 cases to the F128 quarantine lane instead of ordinary-C repair.
- Treat named-case-only repairs as route drift, not progress.
- The exact generated bucket map containing the literal 503 compile-fail,
  57 other-fail, and 7 segfault counts remains missing/ambiguous; the freshest
  concrete no-diagnostic family map found has 265 rows and should not be
  overclaimed as the complete 567-row map.
- `src/20001026-1.c` is classified as `integer_div_rem` in the July 3 family
  map but now reproduces at the prepared move-bundle classifier boundary; do
  not claim this row as a clean integer-div/rem lowering representative without
  additional evidence.
- `unsupported_instruction_fragment` proves the failure reaches RV64 object
  lowering after BIR/prepared/MIR dumps succeed, but it does not identify the
  exact unsupported instruction. Treat these rows as classification evidence,
  not implementation-ready opcode repairs.
- No representative produced a runtime mismatch, timeout, segmentation fault,
  or test-infrastructure failure in this packet.
- Do not route the `src/20000910-1.c` quarantine caveat into ordinary-C repair.
- Do not treat `ideas/open/570_rv64_unsupported_instruction_fragment_owner_diagnostics.md`
  as a capability-repair idea; it is explicitly evidence-enabling.

## Proof

Plan-owner proof:

- `git diff --check -- todo.md ideas/open`
- `scripts/plan_review_state.py show`
