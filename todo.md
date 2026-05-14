Status: Active
Source Idea Path: ideas/open/226_aarch64_module_phoenix_review_replacement_layout.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Write Stage 2 To Stage 3 Handoff

# Current Packet

## Just Finished

Step 5: Write Stage 2 To Stage 3 Handoff added
`src/backend/mir/aarch64/module/stage2_to_stage3_handoff.md` as the Stage 3
intake contract. The handoff records trusted Stage 1 evidence, the no-repair
Stage 1 dependency result, the mandatory Stage 3 draft files from
`stage2_review_layout.md`, route constraints for hierarchical MIR carriers,
lightweight provenance, shared `mir_printer` traversal with AArch64-owned
rendering, and Stage 3 non-goals/rejection signals.

## Suggested Next

Delegate Step 6 final acceptance check: verify the Stage 2 review/layout and
handoff artifacts exist, match each other, preserve the no-implementation-edit
route constraint, and record final Stage 2 proof notes for supervisor review.

## Watchouts

- Preserve the Stage 1 correction: `src/backend/mir/aarch64/module/module.cpp`
  remains compiled legacy evidence until replacement implementation plus build
  wiring exist.
- Stage 2 does not own implementation edits, build rewiring, test expectation
  changes, or replacement draft contents.
- Step 2 classifies the returned-`add`/`sub` path as an optional fast path with
  overfit pressure; do not expand it case by case during replacement layout.
- Treat `FunctionRecord::machine_nodes`, legacy register-name parsing,
  spelling-based identity recovery, raw source/prepared pointers, and snapshot
  booleans as compatibility evidence, not as primary replacement contracts.
- Step 5 wrote the Stage 3 intake contract only; replacement draft contents
  still belong to Stage 3, not Stage 2.
- The mandatory header set is intentionally limited to `module.hpp.md` by the
  phoenix header policy. `helper.hpp` is only an allowed exception if Stage 3
  explicitly proves a private helper header is needed.
- Keep the parent-224 constraints from Step 3 visible when naming draft files:
  canonical MIR nodes are the carrier, flat vectors are compatibility only,
  cached display strings are diagnostics only, and the shared printer owns
  traversal while AArch64 owns rendering.

## Proof

No build needed for this markdown-only handoff packet. The delegated proof
wrote concise read-only `rg` output to `test_after.log`, verifying that
`stage2_to_stage3_handoff.md` exists, contains sections for trustworthy
evidence, evidence repair status, mandatory Stage 3 draft files, route
constraints, non-goals, rejection signals, and sequencing dependencies, and
references all mandatory Stage 3 draft files from `stage2_review_layout.md`.
