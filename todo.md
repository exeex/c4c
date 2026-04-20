# Execution State

Status: Active
Source Idea Path: ideas/open/58_semantic_lir_to_bir_gap_closure_for_x86_backend.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Audit The Active Semantic-Lowering Family
Plan Review Counter: 0 / 10
# Current Packet

## Just Finished

Activation created the idea-58 runbook. No executor packet has run yet.

## Suggested Next

Audit the still-owned semantic `lir_to_bir` failure family, choose one
concrete upstream lowering seam, and pick a narrow proof subset around that
seam.

## Watchouts

- Keep this runbook on upstream semantic/BIR gaps; route stack/addressing
  ownership to idea 62 when that is the more precise failure family.
- Do not add x86 emitter special cases for cases that still fail before
  prepared emission exists.
- Preserve downstream routing when a case graduates into ideas 59, 60, or 61
  instead of silently keeping it under idea 58.

## Proof

Not run yet for the newly activated plan.
