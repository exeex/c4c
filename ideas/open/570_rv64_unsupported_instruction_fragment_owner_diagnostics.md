# RV64 Unsupported Instruction Fragment Owner Diagnostics

Status: Open
Type: Evidence-enabling diagnostic improvement
Parent: `ideas/closed/549_rv64_runtime_and_no_diagnostic_triage.md`
Owning Layer: RV64 object-route diagnostics

## Goal

Replace the generic RV64 object-route `unsupported_instruction_fragment`
evidence gap with diagnostics that identify the unsupported BIR instruction,
operation kind, value type, function, block, and prepared authority context
needed to split durable implementation follow-ups.

## Why This Exists

Step 2 of the 549 triage runbook found nine representative rows that all
successfully dump BIR, prepared BIR, and MIR, then fail only at the RV64 object
runner with:

```text
unsupported_instruction_fragment: BIR instruction requires unsupported RV64
object lowering
```

Representatives:

- `src/20030408-1.c`
- `src/20000412-2.c`
- `src/20071211-1.c`
- `src/pr51933.c`
- `src/pr56982.c`
- `src/20000605-1.c`
- `src/pr78438.c`
- `src/20000622-1.c`
- `src/20000819-1.c`

Evidence:

- `build/agent_state/549_step2_first_owner_classification/classification.tsv`
- `build/agent_state/549_step2_first_owner_classification/stage_matrix.tsv`
- Per-row `object-route.log`, `dump-bir.txt`, and `dump-prepared-bir.txt`
  under `build/agent_state/549_step2_first_owner_classification/`.

The current evidence proves these rows reach RV64 object lowering. It does not
prove a specific opcode repair, prepared authority repair, inline-asm repair,
call-boundary repair, or runtime mismatch. This idea is therefore diagnostic
and evidence-enabling, not a direct capability-repair bucket.

## In Scope

- Add or improve RV64 object-route diagnostics for
  `unsupported_instruction_fragment` so each failure names the first unsupported
  instruction and enough context to assign a durable owner.
- Preserve structured evidence for function name, block/index when available,
  BIR instruction kind/opcode, result/source types, and relevant prepared
  authority or storage context.
- Re-run the nine Step 2 representatives and record the refined diagnostics.
- Split later implementation ideas only after the refined diagnostics prove a
  specific high-confidence owner family.

## Out Of Scope

- Implementing the unsupported RV64 lowering operations discovered by the
  diagnostics.
- Treating the nine rows as one broad RV64 capability repair bucket.
- F128 quarantine work.
- Prepared move-bundle classifier repair for `src/20001026-1.c`.
- Runtime comparison, expected output, unsupported-marker, or allowlist
  changes.

## Acceptance Criteria

- Each of the nine representatives emits a more specific diagnostic than the
  current generic `unsupported_instruction_fragment` message.
- The diagnostic names enough first-bad-fact context for a reviewer to decide
  whether the owner is RV64 object lowering, prepared authority, BIR producer,
  inline asm, call boundary, or another explicit evidence gap.
- The resulting evidence is recorded in a follow-up artifact under
  `build/agent_state`.
- No testcase is claimed as fixed solely because diagnostics changed.

## Reviewer Reject Signals

- Reject a patch that claims RV64 capability progress from diagnostic-only
  changes.
- Reject treating the nine representatives as a single opcode or lowering
  repair without refined first-bad-fact evidence.
- Reject named-case matching for the listed source files instead of improving
  the generic diagnostic path.
- Reject expectation rewrites, unsupported-marker additions, or allowlist
  edits that reduce failure counts without preserving first-owner evidence.
- Reject diagnostics that merely rename `unsupported_instruction_fragment`
  while still omitting the unsupported instruction, function/block context, or
  prepared authority context needed for follow-up ownership.
