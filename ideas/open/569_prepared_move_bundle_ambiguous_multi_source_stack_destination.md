# Prepared Move-Bundle Ambiguous Multi-Source Stack Destination

Status: Open
Type: Prepared contract repair
Parent: `ideas/open/549_rv64_runtime_and_no_diagnostic_triage.md`
Owning Layer: Prepared move-bundle classifier

## Goal

Repair the prepared move-bundle classifier boundary for the
`ambiguous_non_parallel_multi_source_stack_destination` shape reproduced by
`src/20001026-1.c`.

## Why This Exists

Step 2 of the 549 triage runbook classified `src/20001026-1.c` as a
high-confidence prepared contract/classifier failure, not an RV64 integer
division or remainder lowering failure.

Evidence:

- `build/agent_state/549_step2_first_owner_classification/src_20001026-1.c/object-route.log`
- `build/agent_state/549_step2_first_owner_classification/src_20001026-1.c/dump-bir.txt`
- `build/agent_state/549_step2_first_owner_classification/src_20001026-1.c/dump-prepared-bir.txt`
- `build/agent_state/549_step2_first_owner_classification/classification.tsv`

The first explicit diagnostic is:

```text
prepared_consumer_category=ambiguous_non_parallel_multi_source_stack_destination:
prepared move-bundle classifier rejected ambiguous non-parallel multi-source
stack-destination authority
```

The row was selected from an `integer_div_rem` family map, but the reproduced
first owner is the prepared classifier boundary. Treat integer div/rem lowering
as out of scope unless fresh evidence proves the prepared classifier boundary
has been crossed.

## In Scope

- Identify the prepared move bundle that produces the ambiguous non-parallel
  multi-source stack-destination diagnostic for `src/20001026-1.c`.
- Decide whether the prepared classifier should publish a coherent authority
  for this shape or split it into explicit supported moves.
- Add focused prepared-layer coverage for the ambiguous multi-source
  stack-destination contract.
- Prove that the row advances past the prepared classifier boundary without
  weakening the object runner, expected output, unsupported markers, or
  no-diagnostic accounting.

## Out Of Scope

- RV64 integer division or remainder instruction lowering.
- Generic `unsupported_instruction_fragment` cleanup.
- Runtime-output comparison changes.
- Expectation rewrites, allowlist pruning, or unsupported-marker downgrades.
- Broad prepared move-bundle rewrites unrelated to this ambiguous
  multi-source stack-destination shape.

## Acceptance Criteria

- The prepared classifier no longer rejects the representative solely with
  `ambiguous_non_parallel_multi_source_stack_destination`.
- Focused tests or dumps prove the prepared contract now names coherent
  authority for the multi-source stack-destination shape or an intentional
  supported split.
- `src/20001026-1.c` is rerun through the RV64 gcc_torture object runner and
  the result is recorded. If it reaches a later RV64 lowering failure, that
  later failure is classified separately rather than claimed as closure.
- No runtime comparison, expected output, unsupported marker, or allowlist
  behavior is weakened.

## Reviewer Reject Signals

- Reject a patch that claims integer div/rem progress while the first failure
  remains the prepared classifier diagnostic above.
- Reject named-case shortcuts that special-case `src/20001026-1.c` instead of
  repairing the prepared multi-source stack-destination classification rule.
- Reject expectation changes, unsupported-marker additions, or allowlist edits
  that turn the row green without crossing the prepared classifier boundary.
- Reject broad prepared move-bundle rewrites that do not include focused proof
  for the ambiguous non-parallel multi-source stack-destination contract.
- Reject a route that merely renames the diagnostic while leaving the same
  prepared boundary failure hidden behind a new wrapper.
