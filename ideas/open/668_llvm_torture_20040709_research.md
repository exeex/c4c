# LLVM Torture 20040709 Owner Discovery

Status: Open
Type: Research and architecture documentation
Parent: `ideas/open/658_backend_baseline_history_umbrella_triage.md`
Related:
- `docs/backend_baseline_history_triage/failure_classification.md`
- `docs/backend_baseline_history_triage/follow_up_order.md`
Owning Layer: LLVM torture integration owner discovery
Queue Order: 68

## Goal

Produce concrete research documents under
`docs/llvm_torture_20040709_owner_discovery/` that decide whether
`llvm_gcc_c_torture_src_20040709_2_c` and
`llvm_gcc_c_torture_src_20040709_3_c` share an existing backend owner or need
a separate frontend, runtime, backend, or harness follow-up idea.

## Why This Exists

Step 2 intentionally deferred the two LLVM torture rows because current
evidence did not prove a first implementation owner. Folding them into an RV64
or prepared backend follow-up would risk testcase-overfit and mixed ownership.

## Research Questions And Required Answer Files

There are two research questions. The delivery must contain exactly two
question-answer Markdown files, one for each question, plus one `index.md`.
Each answer file must answer only its assigned question and may link to the
other answer file for supporting context.

1. `01_current_failure_boundary.md`

   Question: What is the current first observable failure boundary for the
   `20040709_2.c` and `20040709_3.c` LLVM torture rows?

   Required answer shape:
   - cite the current baseline log and any focused reproduction command used
   - classify frontend, prepared backend, target backend, runtime, or harness
     boundary for each row
   - state whether either row has enough evidence for implementation

2. `02_owner_mapping.md`

   Question: Do the two LLVM torture rows map to an existing generated
   follow-up idea, or do they require a new direct implementation idea?

   Required answer shape:
   - compare each row against the generated Step 3 follow-up ideas
   - name any shared owner only with concrete evidence
   - propose a separate direct implementation idea only if no existing owner
     fits

## Required Documentation Output

Create the research documents in:

```text
docs/llvm_torture_20040709_owner_discovery/
```

Required files:

- `docs/llvm_torture_20040709_owner_discovery/index.md`
- `docs/llvm_torture_20040709_owner_discovery/01_current_failure_boundary.md`
- `docs/llvm_torture_20040709_owner_discovery/02_owner_mapping.md`

`index.md` must link to all numbered answer files and summarize the overall
result. It must not replace any required answer file.

## In Scope

- Research and evidence gathering for current LLVM torture rows 1941 and
  1942.
- Classification of the first observable owner boundary for each row.
- Mapping to an existing follow-up idea only when evidence proves the same
  first owner.

## Out Of Scope

- Implementation changes.
- Test expectation, unsupported-marker, allowlist, timeout, runtime behavior,
  or baseline acceptance changes.
- Claiming either LLVM torture row is repaired by a backend family without
  focused evidence.
- Activating this idea into `plan.md` unless explicitly requested later.

## Acceptance Criteria

- `docs/llvm_torture_20040709_owner_discovery/` contains one `index.md` plus
  exactly the two numbered answer files listed above.
- Each answer file answers its assigned question directly and cites concrete
  logs, commands, code surfaces, or generated follow-up ideas.
- The research states whether rows 1941 and 1942 map to an existing follow-up
  idea or require a new direct implementation idea.
- No implementation files, test expectations, unsupported markers, allowlists,
  runtime behavior, active plan state, or lifecycle history are changed.

## Reviewer Reject Signals

- Reject implementation, expectation, unsupported-marker, allowlist, timeout,
  runtime behavior, or baseline acceptance changes under this research idea.
- Reject assigning the LLVM torture rows to RV64, prepared CLI, AArch64, or
  object-emission owners without focused evidence for each row.
- Reject answer-file count mismatches or replacing numbered answer files with
  only an index summary.
- Reject named-case shortcuts or harness filtering that hides the LLVM torture
  failures.
- Reject broad follow-up recommendations that do not separate documentation,
  direct implementation ideas, and discussion-required architecture work.
