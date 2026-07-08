# RV64 gcc_torture 1000-Pass Recovery Umbrella

Status: Open
Type: Umbrella triage and follow-up idea generator
Parent: `ideas/closed/420_rv64_gcc_torture_post_contract_umbrella.md`
Handoff Directory: `docs/rv64_gcc_torture_1000_pass_recovery/`
Related:
- `ideas/closed/420_rv64_gcc_torture_post_contract_umbrella.md`
- `ideas/closed/587_prepared_value_freshness_authority_mvp.md`
- `ideas/closed/588_shared_prealloc_move_operand_source_freshness_inventory.md`
- `ideas/closed/589_direct_edge_publication_move_freshness_ownership.md`
- `ideas/closed/590_branch_stack_load_freshness_contract.md`
- `ideas/closed/592_typed_aggregate_branch_stack_source_publication.md`
- `ideas/closed/593_rv64_branch_stack_source_freshness_consumption.md`
- `ideas/closed/594_rv64_branch_stack_source_consumption_followup_from_593.md`
- `ideas/closed/595_prepared_value_architecture_followup_umbrella.md`
- `ideas/closed/596_pointer_rhs_branch_stack_source_policy_publication.md`
- `ideas/closed/597_pointer_address_semantic_model_research.md`
- `ideas/closed/598_select_carrier_alias_freshness_contract.md`
- `ideas/closed/599_pointer_base_plus_offset_selected_authority.md`
- `ideas/closed/600_pointer_value_memory_use_freshness_authority.md`
- `build/agent_state/rv64_gcc_c_torture_backend_summary.tsv`
- `build/agent_state/rv64_gcc_c_torture_backend_failed.txt`

## Goal

Use the fresh RV64 gcc_torture backend-object scan at `470/1467` passing to
classify the remaining `997` failures and generate an ordered follow-up idea
queue aimed at reaching at least `1000` passing cases.

This umbrella does not implement fixes directly. It must identify the next
high-yield capability families, split them by first owning layer, and produce
durable follow-up ideas whose combined expected impact can plausibly close the
gap from `470` passing cases to `1000+`.

The `1000+` pass count is a direction marker, not permission to force
implementation through unclear architecture. If classification shows the next
large gains are blocked by weak or unsettled architecture, the umbrella should
surface those weaknesses as discussion, research, or design follow-up ideas
and may close without a concrete implementation queue that plausibly reaches
`1000+`.

## Why This Exists

The previous RV64 gcc_torture umbrella closed around a stable `349/1467` pass
baseline. Since then, the project completed a substantial prepared-value and
RV64 freshness/authority round, including branch stack-source publication and
consumption, select-carrier alias freshness, pointer base-plus-offset
authority, and pointer-value memory-use freshness.

A fresh scan now reports:

```text
total=1467 passed=470 failed=997
```

That is real movement, but it is not yet enough. The next objective should not
be one more narrow testcase repair. The project needs another evidence-driven
umbrella pass that turns the remaining failures into a prioritized backlog and
keeps implementation work focused on broad semantic capability gains until the
RV64 gcc_torture backend-object path exceeds `1000` passing cases.

Direct implementation would be premature because the remaining failures likely
mix BIR semantic producer gaps, prepared authority gaps, RV64/MIR consumer
gaps, ABI/runtime issues, unsupported instruction fragments, aggregate and
variadic families, string/library calls, floating-point support, and
diagnostic-only failures. The umbrella must classify first so follow-up ideas
do not mix owners or overfit named cases.

## Current Evidence

Authoritative scan for this umbrella:

- Command:
  `scripts/check_progress_rv64_gcc_c_torture_backend.sh`
- Result:
  `1467` total, `470` passed, `997` failed, `0` missing
- Summary:
  `build/agent_state/rv64_gcc_c_torture_backend_summary.tsv`
- Failed list:
  `build/agent_state/rv64_gcc_c_torture_backend_failed.txt`
- Per-case logs:
  `build/rv64_gcc_c_torture_backend/`

This scan supersedes the 2026-07-02 `349/1467` baseline from
`ideas/closed/420_rv64_gcc_torture_post_contract_umbrella.md` for choosing the
next recovery queue. The `349` baseline remains useful only as historical
evidence that the current state gained `121` passing cases.

The umbrella should also consider the latest closed architecture notes from
ideas 587 through 600, because many old failure buckets may have changed owner
after selected freshness and pointer/address authority work landed.

The first triage pass must explicitly look for failures that are now close to
repair because the recent architecture round already supplied the shared
contract. Prefer buckets where the likely fix is to extend an existing
selected-authority family, publish a missing same-shape fact, or wire an
RV64/MIR consumer to authority that is already produced. Those rows should be
ranked ahead of unrelated new architecture work with similar raw counts.

The second triage pass must ask the opposite question: after the architecture
round, what remaining failures expose architecture weak points rather than
simple missing wiring? These weaknesses must be named explicitly. If a weak
point needs human architecture discussion before a durable implementation
idea, the umbrella should create a research/discussion idea or record it as an
intentional closure finding instead of forcing a code route.

## In Scope

- Create or refresh handoff documents under
  `docs/rv64_gcc_torture_1000_pass_recovery/`.
- Reclassify the `997` current failures by first owning layer.
- Compare the current `470/1467` evidence against the older `349/1467`
  umbrella baseline and record which broad families appear improved, stale, or
  still dominant.
- Bucket failures by likely capability family using logs, diagnostics,
  generated object/runtime status, and current source case families.
- Generate ordered follow-up ideas under `ideas/open/`.
- Prefer follow-up ideas whose expected impact is broad enough to plausibly
  move the pass count by dozens or hundreds, not one or two cases.
- Identify architecture weak points exposed by the remaining failures after
  the 587 through 600 architecture round.
- Generate discussion or research ideas for unsettled architecture questions
  that should be talked through before implementation.
- Record the dependency order needed to reach `1000+` pass without letting
  target-local RV64 work bypass missing shared producer or prepared authority.
- Identify explicit quarantine/defer lanes for low-yield or policy-heavy
  families such as F128, library/string calls, unsupported builtins, or
  feature families whose first owner is not yet clear.

## Out Of Scope

- Implementing RV64, BIR, prepared, ABI, test, expectation, or harness fixes
  inside this umbrella.
- Treating RV64 gcc_torture as default CTest coverage.
- Changing the allowlist, unsupported markers, expected output, runtime
  comparison, pass/fail accounting, default harness behavior, or timeout
  policy as proof of progress.
- Opening mixed-owner follow-ups that combine BIR producer repair,
  prepared/prealloc authority, RV64 target consumption, runtime mismatch, and
  test infrastructure in one implementation idea.
- Selecting follow-ups purely because a testcase name is familiar from recent
  work.

## Priority Model

Order generated follow-up ideas by expected broad pass-count impact, first
owning layer, and semantic leverage:

1. High-frequency failures that can reuse or slightly extend the architecture
   just completed in ideas 587 through 600: selected freshness authority,
   branch stack-source publication/consumption, select-carrier alias
   freshness, pointer base-plus-offset authority, and pointer-value memory-use
   freshness.
2. Cases where shared authority is already produced but an RV64/MIR consumer
   has not been wired to consume it.
3. High-frequency BIR semantic producer or prepared-authority gaps that block
   many ordinary C cases before RV64 lowering can be correct.
4. Unsupported instruction-fragment families with broad ordinary-C coverage,
   especially if they are not F128-only or library-only.
5. Runtime mismatch families where the object route emits and links but
   aborts, segfaults, or produces wrong output.
6. ABI, aggregate, variadic, stack-frame, and memory-layout families only
   after the logs show a coherent first owner.
7. Floating-point, vector, string/library, builtin, and F128-heavy families
   should be ranked by observed breadth and ordinary-C leverage; F128 remains
   low priority unless it gates broad non-F128 progress.

The umbrella should prefer fewer high-yield follow-up ideas over many
case-shaped slices. The follow-up queue should explicitly explain how it could
move from `470` to `1000+` passing cases, even if the exact pass gain cannot
be guaranteed.

## Required Handoff Documents

The umbrella must produce these documents:

- `docs/rv64_gcc_torture_1000_pass_recovery/index.md`
- `docs/rv64_gcc_torture_1000_pass_recovery/current_scan_summary.md`
- `docs/rv64_gcc_torture_1000_pass_recovery/failure_bucket_map.md`
- `docs/rv64_gcc_torture_1000_pass_recovery/high_yield_followup_plan.md`
- `docs/rv64_gcc_torture_1000_pass_recovery/dependency_order_to_1000.md`

`current_scan_summary.md` must record the command, paths, total/pass/fail
counts, and relationship to the older `349/1467` baseline.

`failure_bucket_map.md` must classify the current failures by first owner and
capability family. It must not only list raw counts.

`high_yield_followup_plan.md` must identify the follow-up ideas to generate,
their expected breadth, and why each is not testcase-overfit.

`dependency_order_to_1000.md` must state the recommended activation order,
which ideas must run before target consumers, and which deferred families are
not on the main route to `1000+`.

## Required Follow-Up Idea Families

Unless fresh evidence proves a better split, the umbrella must decide whether
to generate or explicitly defer these follow-up families:

- The largest current ordinary-C failure bucket after reclassification.
- The largest current BIR semantic producer gap.
- The largest current prepared/prealloc authority or publication gap.
- The largest current RV64/MIR consumer gap where upstream authority already
  exists.
- The largest current "architecture already exists, missing extension or
  wiring" bucket from the ideas 587 through 600 authority/freshness families.
- The largest current runtime mismatch family.
- The largest current unsupported instruction-fragment family.
- The largest current aggregate/variadic/ABI family.
- The largest current architecture-weakness family that should be discussed or
  researched before implementation rather than forced into a code slice.
- A quarantine or defer policy for low-yield F128, string/library, builtin,
  or environment-dependent families if they would distract from the `1000+`
  route.

Each generated follow-up idea must name its owning layer, prerequisites,
estimated evidence breadth, proof surface, acceptance criteria, and reviewer
reject signals.

## Acceptance Criteria

- The handoff directory contains `index.md` plus all four required documents.
- The handoff docs agree on the same current `470/1467` scan evidence and do
  not cite stale `349/1467` counts as current.
- The `997` failures are classified by first owner and capability family.
- The umbrella generates an ordered follow-up idea queue under `ideas/open/`.
- The follow-up queue explains how the recommended sequence is intended to
  move RV64 gcc_torture toward `1000+` passing cases.
- If the scan shows the next high-yield gains are blocked by architecture
  weakness, the umbrella names those weak points and may close with
  discussion/research follow-ups instead of claiming a forced implementation
  path to `1000+`.
- Each generated follow-up idea names its owning layer and avoids mixed
  producer/consumer ownership.
- The umbrella records which families are intentionally deferred or
  quarantined and why they are not on the first `1000+` route.
- The umbrella makes no implementation, test expectation, unsupported marker,
  allowlist, runtime behavior, timeout, or default harness changes.

## Closure Note Requirements

The closure note must state:

1. Which scan evidence was used and whether a newer scan superseded the
   `470/1467` result.
2. How the remaining failures were bucketed by first owner and capability
   family.
3. Which follow-up ideas were generated, in dependency order.
4. Which generated ideas are expected to be high-yield for the path to
   `1000+` pass.
5. Which architecture weak points were exposed by the remaining failures and
   whether they require discussion before implementation.
6. Which families were deferred or quarantined, and why.
7. Whether any stale handoff docs, summaries, or historical counts remain and
   how they should be interpreted.
8. The next recommended lifecycle activation after this umbrella closes.

## Reviewer Reject Signals

- Reject direct implementation inside this umbrella idea.
- Reject output that only reports `470/1467` without row-level ownership and
  follow-up ideas.
- Reject stale `349/1467`, `404/1063`, or other historical counts being used
  as current evidence after the fresh scan exists.
- Reject follow-up ideas that mix BIR producer repair, prepared authority,
  RV64/MIR consumer migration, runtime mismatch, and test infrastructure in
  one implementation slice.
- Reject testcase-shaped shortcuts, named-case-only fixes, fallback-name
  recovery, raw BIR shape matching, target-local inference, expectation
  rewrites, unsupported downgrades, allowlist filtering, timeout tuning, or
  weaker runtime checks as progress.
- Reject claiming progress toward `1000+` through one-off case repairs whose
  nearby same-feature cases remain unclassified.
- Reject forcing an implementation idea when the failure bucket exposes an
  unsettled architecture question that should be discussed or researched first.
- Reject treating stack homes, layout facts, target operand shape, diagnostic
  output, candidate counts, or printed assembly as semantic authority without
  selected shared authority or a clearly owned target-consume contract.
