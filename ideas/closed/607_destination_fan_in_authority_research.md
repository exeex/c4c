# Destination Fan-In Authority Research

Status: Closed
Type: Research and architecture documentation
Parent: `ideas/open/601_rv64_gcc_torture_1000_pass_recovery_umbrella.md`
Related:
- `ideas/closed/587_prepared_value_freshness_authority_mvp.md`
- `ideas/closed/588_shared_prealloc_move_operand_source_freshness_inventory.md`
- `docs/rv64_gcc_torture_1000_pass_recovery/failure_bucket_map.md`
Owning Layer: prepared/prealloc authority
Queue Order: 6
Prerequisites: none for research; implementation must wait for this idea's conclusion
Estimated Evidence Breadth: `125` non-parallel multi-source stack-destination rows
Proof Surface: documentation under `docs/destination_fan_in_authority/`, citing current logs and prepared/prealloc authority surfaces

## Goal

Produce concrete research documents under `docs/destination_fan_in_authority/`
that define whether and how non-parallel move bundles may choose among
multiple candidate stack destinations.

## Why This Exists

The `125` destination fan-in rows are high-count, but current selected
freshness work mostly proves source validity. A durable implementation needs
an ordering, mutual-exclusion, or merge-authority rule for destinations before
target materialization.

## Research Questions And Required Answer Files

There are three research questions. The delivery must contain exactly three
question-answer Markdown files plus one `index.md`.

1. `01_current_failure_shapes.md`

   Question: Which current logs and diagnostics make up the non-parallel
   multi-source stack-destination family?

   Required answer shape:
   - representative row table from the July 8 scan
   - diagnostic vocabulary and prepared/prealloc surfaces involved
   - conclusion on which rows belong to this family

2. `02_destination_authority_rule.md`

   Question: What destination legality rule is needed: ordering,
   mutual-exclusion, merge authority, or explicit rejection?

   Required answer shape:
   - alternatives considered
   - required producer facts for each viable alternative
   - selected rule or explicit unresolved decision

3. `03_implementation_split.md`

   Question: If implementable, which single-owner follow-up ideas should be
   opened after the rule is accepted?

   Required answer shape:
   - producer-authority work separated from RV64 consumption
   - proof surfaces for any proposed implementation ideas
   - rows that must remain rejected until the rule exists

## Required Documentation Output

Create the research documents in:

```text
docs/destination_fan_in_authority/
```

Required files:

- `docs/destination_fan_in_authority/index.md`
- `docs/destination_fan_in_authority/01_current_failure_shapes.md`
- `docs/destination_fan_in_authority/02_destination_authority_rule.md`
- `docs/destination_fan_in_authority/03_implementation_split.md`

## In Scope

- Architecture research and documentation only.
- Current-log evidence for the `125` row family.
- Clear implementation split recommendations if a rule is found.

## Out Of Scope

- Implementation changes.
- RV64 target shortcuts for destination choice.
- Test expectations, unsupported markers, allowlists, runtime behavior,
  timeout policy, or lifecycle activation.

## Acceptance Criteria

- `docs/destination_fan_in_authority/` contains one `index.md` plus exactly
  the three numbered answer files above.
- The documents cite concrete current logs, diagnostics, and code surfaces.
- The conclusion either defines an implementation-ready destination authority
  rule or records why implementation must remain blocked.
- No implementation, expectation, unsupported-marker, allowlist, runtime,
  timeout, accounting, or active lifecycle files are changed.

## Completion Notes

Closed on 2026-07-08 after the documentation package was completed under
`docs/destination_fan_in_authority/` with exactly the required index and three
answer files. The package selects explicit rejection until prepared/prealloc
producer authority publishes exactly one destination authority contract;
implementation remains blocked for this family until that follow-up idea exists.

## Reviewer Reject Signals

- Reject implementation work disguised as research.
- Reject answer-file count mismatches or collapsed questions.
- Reject selecting a destination by testcase shape, source order accident, or
  final assembly convenience without an explicit authority rule.
- Reject RV64 consumer work that bypasses prepared/prealloc destination
  authority.
- Reject broad recommendations that do not separate producer authority from
  target consumption.
