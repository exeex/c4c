Status: Active
Source Idea Path: ideas/open/449_pointer_relational_branch_publication.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Audit Pointer Relational Branch Evidence

# Current Packet

## Just Finished

Activated the runbook for Step 1 of the pointer relational branch publication
plan.

## Suggested Next

Execute Step 1: inspect `build/agent_state/441_step4_residual_disposition/`,
with focus on `20010329-1` pointer relational fused compare
`compare=uge ptr %t5, %t7` and `930930-1` pointer relational branch
candidates. Classify each row by predicate, operand types, operand homes,
provenance/null evidence, result home, prepared branch-condition fact, target
labels, authority state, and first missing policy, producer, or consumer fact.

## Watchouts

- Keep this plan limited to pointer relational fused branch predicates.
- Do not reopen pointer `eq/ne` branch publication; closed idea 441 owns that
  route.
- Do not fold select-result branch publication into this plan; that belongs to
  `ideas/open/450_select_result_branch_publication.md`.
- Do not infer pointer relational semantics from raw BIR compare/branch shape,
  block order, filenames, function names, testcase names, or one dump layout.
- Keep missing, incoherent, unsupported, or policy-ambiguous relational branch
  authority fail-closed.
- Do not accept or modify `test_baseline.new.log`.
- Do not touch `test_before.log`, `test_after.log`, or `review/`.

## Proof

Activation-only validation:

```sh
git diff --check
```
