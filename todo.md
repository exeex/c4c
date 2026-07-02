Status: Active
Source Idea Path: ideas/open/545_bir_semantic_producer_admission_reconstruction.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Reconstruct Current BIR Admission Rows

# Current Packet

## Just Finished

Lifecycle activation selected the BIR semantic producer admission
reconstruction idea and transcribed it into `plan.md`.

## Suggested Next

Execute Step 1 from `plan.md`: reconstruct the current `semantic lir_to_bir`
candidate row set from the 2026-07-02 RV64 gcc_torture backend scan artifacts,
record the evidence artifact path, and state whether a verified current row
set exists.

## Watchouts

- Do not use stale historical counts as current row evidence.
- Do not implement RV64/MIR workarounds or infer facts missing from the BIR
  producer.
- Do not weaken semantic admission checks, expectations, unsupported markers,
  allowlists, or runtime comparison behavior.
- Keep F128-primary rows in the quarantine lane, not ordinary-C producer
  cleanup.

## Proof

Lifecycle-only activation. No build or compile proof was required.
