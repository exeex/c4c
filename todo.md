Status: Active
Source Idea Path: ideas/open/560_bir_scalar_signature_control_semantic_producer_admission.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Inspect Producer Boundary And Decide Split

# Current Packet

## Just Finished

Lifecycle activation created `plan.md` from the source idea and initialized
this execution scratchpad for Step 1.

## Suggested Next

Execute Step 1: inspect the 20-row scalar/signature/control evidence, trace
each topic to its BIR semantic producer/admission boundary, and record whether
the lane can stay unified or must be split before implementation.

## Watchouts

- Do not implement before the split/no-split decision is backed by row and code
  evidence.
- Do not route function-signature rows to ABI/RV64 lowering before proving BIR
  publication correctness.
- Do not claim progress through expectation rewrites, unsupported downgrades,
  allowlist changes, or named-case shortcuts.

## Proof

Lifecycle-only activation; no build or test proof required yet.
