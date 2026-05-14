Status: Active
Source Idea Path: ideas/open/227_aarch64_module_phoenix_replacement_drafts.md
Source Plan Path: plan.md
Current Step ID: 7
Current Step Title: Review Replacement Draft Set

# Current Packet

## Just Finished

Completed plan Step 7 by reviewing the complete Stage 3 replacement draft set
against `stage2_review_layout.md`, `stage2_to_stage3_handoff.md`, and parent
224 constraints. Wrote
`src/backend/mir/aarch64/module/stage3_draft_review.md` with an explicit accept
decision for implementation conversion and no mandatory repair blockers.

## Suggested Next

Delegate the next plan step to convert the accepted Stage 3 draft set into the
authorized implementation form, preserving the draft dependency order and
header policy.

## Watchouts

- Review accepted the draft set, but implementation conversion must not drift
  into preserving the legacy catch-all record assembler.
- Keep `FunctionRecord::machine_nodes`, broad inspection records, cached
  display strings, source spellings, raw source/prepared views, and register
  strings as compatibility or diagnostic outputs only.
- Keep public assembly routed through the shared `mir_printer`; AArch64 should
  own target rendering hooks only.
- Do not introduce component-level public headers or a target render API named
  `__repr__`.

## Proof

Markdown-only proof written to `test_after.log`. The proof confirms every
mandatory Stage 3 draft exists, only `module.hpp.md` exists among `.hpp.md`
drafts, the review artifact records accept/block status, Stage 2 handoff
alignment, parent 224 constraints, no cached display or flat-vector authority,
no `__repr__` API, no real source/build/test edits, and no blockers or repair
needs. No build was required for this markdown-only review packet.
