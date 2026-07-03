# Current Packet

Status: Active
Source Idea Path: ideas/open/546_rv64_instruction_fragment_current_classification.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Reconstruct Current Instruction-Fragment Rows

## Just Finished

Activated the runbook from `ideas/open/546_rv64_instruction_fragment_current_classification.md`.

## Suggested Next

Execute Step 1: reconstruct the current `unsupported_instruction_fragment` row table from the 2026-07-02-aligned summary and per-case logs before using any historical taxonomy.

## Watchouts

- Leave `review/557_step13_vector_local_memory_review.md` untouched.
- Do not reuse stale 190-row or 82-row instruction-fragment counts as current scope.
- Do not implement RV64 lowering, edit expectations, or weaken unsupported markers in this classification packet.
- Screen primary-F128 rows into the quarantine lane before ordinary-C bucket ranking.

## Proof

- Activation is lifecycle-only; no build proof required.
