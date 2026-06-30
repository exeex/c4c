# RV64 Instruction Fragment Bucket Classification

Status: Open
Type: Umbrella child analysis idea
Parent: `ideas/open/420_rv64_gcc_torture_post_contract_umbrella.md`
Owning Layer: RV64/MIR triage
Handoff Directory: `docs/rv64_gcc_torture_post_contract/`

## Goal

Split the largest RV64 gcc_torture failure family,
`unsupported_instruction_fragment`, into high-frequency concrete RV64 lowering
owners, excluding F128 from the primary route.

## Why This Exists

The preserved `try_gcc_torture` scan showed the biggest bucket was generic
RV64 instruction-fragment failure, far larger than any F128-specific bucket.
The exploratory branch spent too long chasing one `conversion.c`/F128 route
instead of decomposing this broad family.

## In Scope

- Re-run or inspect RV64 gcc_torture failure logs from reset `main`.
- Group `unsupported_instruction_fragment` rows by BIR opcode, operand/result
  type, prepared fact completeness, and likely RV64 lowering owner.
- Identify ordinary high-frequency non-F128 rows first: integer operations,
  scalar comparisons, shifts, select/materialization, call-adjacent moves,
  pointer arithmetic, and scalar F32/F64 operations.
- Record representative rows and proposed child ideas in the handoff docs.
- Mark F128 rows as quarantine/low-priority unless they block a broader
  non-F128 owner.

## Out Of Scope

- Implementing instruction lowering.
- Continuing `conversion.c` as the main success metric.
- Repairing BIR/prepared producer gaps in RV64.
- Treating F128 as a priority bucket.

## Acceptance Criteria

- The broad instruction-fragment bucket is split into concrete sub-buckets with
  counts, representative rows, and owning layers.
- At least three high-impact non-F128 follow-up implementation ideas are
  proposed if the evidence supports them.
- F128 rows are explicitly separated and deprioritized.
- Default CTest remains stable.

## Reviewer Reject Signals

- Reject output that picks one testcase row without showing bucket frequency.
- Reject F128-first routing.
- Reject raw BIR or source-name matching as a proposed implementation path.
- Reject classifying BIR/prepared producer gaps as RV64 lowering work.
- Reject expectation, allowlist, or unsupported-marker edits as progress.
