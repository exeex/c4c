# Current Packet

Status: Active
Source Idea Path: ideas/open/255_aarch64_alu_legacy_semantic_lowering_followup.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Audit Legacy ALU Routes Against Current Pipeline

## Just Finished

Activated `ideas/open/255_aarch64_alu_legacy_semantic_lowering_followup.md`
as the active runbook.

## Suggested Next

Delegate Step 1 as a survey/classification packet. The executor should inspect
current ALU lowering, records, and focused tests; classify each legacy route as
accepted, obsolete, or separate-idea material; and recommend the first
implementation packet without editing implementation files.

## Watchouts

- Do not reopen ALU markdown redistribution or recreate `alu.md`.
- Do not revive text-emitter accumulator conventions as semantic authority.
- Do not use named-case shortcuts, final assembly string matching, expectation
  rewrites, or unsupported downgrades to claim progress.
- Keep signed power-of-two division/remainder separate from unsigned reduction
  unless signed semantics are separately designed and proved.
- Preserve explicit 32-bit extension, scratch-conflict, and i128 high-half
  requirements when those routes are classified as accepted.

## Proof

No proof run for activation-only lifecycle work.
