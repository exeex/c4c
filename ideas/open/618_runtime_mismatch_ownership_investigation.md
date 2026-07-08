# Runtime Mismatch Ownership Investigation

Status: Open
Type: Research and architecture documentation
Parent: `ideas/open/601_rv64_gcc_torture_1000_pass_recovery_umbrella.md`
Related:
- `docs/rv64_gcc_torture_1000_pass_recovery/current_scan_summary.md`
- `docs/rv64_gcc_torture_1000_pass_recovery/failure_bucket_map.md`
- `docs/rv64_gcc_torture_1000_pass_recovery/high_yield_followup_plan.md`
Owning Layer: runtime ownership mapping across ABI, layout, memory, call, and runtime support
Queue Order: 17
Prerequisites: best run after compile-time local-memory, global-data, ABI, and RV64/MIR blockers shrink enough to avoid mixed-owner noise
Estimated Evidence Breadth: `72` runtime mismatches plus `3` run timeouts
Proof Surface: documentation under `docs/runtime_mismatch_ownership/`, mapping runtime abort, segfault, wrong-output, and timeout rows to likely first owners

## Goal

Produce concrete research documents under `docs/runtime_mismatch_ownership/`
that map emitted-object runtime failures to likely implementation owners before
any runtime fix is attempted.

## Why This Exists

The current scan has `51` runtime aborts, `21` runtime segfaults, and `3` run
timeouts. These symptoms are past object emission, but they do not identify
whether the root owner is ABI, layout, local/global memory, call lowering, or
true runtime support.

## Research Questions And Required Answer Files

There are three research questions. The delivery must contain exactly three
question-answer Markdown files plus one `index.md`.

1. `01_runtime_symptom_map.md`

   Question: Which current runtime rows abort, segfault, mismatch output, or
   time out, and what concrete artifact shows each symptom?

   Required answer shape:
   - row table grouped by symptom
   - representative logs and commands
   - conclusion on stable symptom families

2. `02_likely_first_owner_map.md`

   Question: Which likely first owner best explains each runtime family after
   compile-time blockers are considered?

   Required answer shape:
   - candidate owners: ABI, layout, local/global memory, call lowering, true
     runtime support, or unresolved
   - evidence for and against each owner
   - rows that need rerun after prerequisite ideas

3. `03_followup_implementation_queue.md`

   Question: Which single-owner implementation ideas, if any, should follow
   this investigation?

   Required answer shape:
   - proposed implementation splits by owner
   - proof surface for each split
   - rows that remain discussion or policy issues

## Required Documentation Output

Create the research documents in:

```text
docs/runtime_mismatch_ownership/
```

Required files:

- `docs/runtime_mismatch_ownership/index.md`
- `docs/runtime_mismatch_ownership/01_runtime_symptom_map.md`
- `docs/runtime_mismatch_ownership/02_likely_first_owner_map.md`
- `docs/runtime_mismatch_ownership/03_followup_implementation_queue.md`

## In Scope

- Runtime failure investigation and ownership mapping.
- Citing concrete current logs, qemu results, abort sites, signals, wrong
  outputs, and likely upstream owners.
- Recommendations for later single-owner ideas.

## Out Of Scope

- Implementation changes.
- Runtime comparison weakening, timeout tuning, allowlist filtering, expected
  output edits, unsupported-marker changes, or active lifecycle transitions.

## Acceptance Criteria

- `docs/runtime_mismatch_ownership/` contains one `index.md` plus exactly the
  three numbered answer files above.
- Runtime rows are mapped by symptom and likely first owner with concrete log
  evidence.
- Any recommended implementation work is split by owner and proof surface.
- No implementation, expectation, unsupported-marker, allowlist, timeout,
  runtime behavior, accounting, or active lifecycle files are changed.

## Reviewer Reject Signals

- Reject implementation work disguised as runtime investigation.
- Reject treating one abort, segfault, or timeout as proof of a runtime fix.
- Reject weakening runtime comparison, timeout policy, pass/fail accounting, or
  allowlist coverage.
- Reject broad runtime recommendations that do not separate ABI, layout,
  memory, call, and true runtime support owners.
- Reject claims that lack concrete current log or execution evidence.
