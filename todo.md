Status: Active
Source Idea Path: ideas/open/226_aarch64_module_phoenix_review_replacement_layout.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Declare Stage 3 Draft Artifact Map

# Current Packet

## Just Finished

Step 4: Declare Stage 3 Draft Artifact Map extended
`src/backend/mir/aarch64/module/stage2_review_layout.md` with the complete
mandatory Stage 3 draft artifact map. The artifact now names the replacement
directory index `module.md`, the single mandatory header draft
`module.hpp.md`, and every mandatory implementation draft:
`module.cpp.md`, `function_traversal.cpp.md`,
`operand_resolution.cpp.md`, `instruction_lowering.cpp.md`,
`branch_control_lowering.cpp.md`, `call_lowering.cpp.md`,
`public_assembly_bridge.cpp.md`, and `compatibility_projection.cpp.md`. It
also states the phoenix header policy and records that no Stage 1 extraction
repair is required before these draft artifacts are trustworthy.

## Suggested Next

Delegate Step 5 handoff work: write the Stage 2 to Stage 3 handoff artifact
that identifies trustworthy evidence, the no-repair evidence dependency
status, mandatory draft files, route constraints, non-goals, and rejection
signals for Stage 3 drift.

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
- Step 4 declared the Stage 3 draft artifact map only; replacement draft
  contents still belong to Stage 3, not Stage 2.
- The mandatory header set is intentionally limited to `module.hpp.md` by the
  phoenix header policy. `helper.hpp` is only an allowed exception if Stage 3
  explicitly proves a private helper header is needed.
- Keep the parent-224 constraints from Step 3 visible when naming draft files:
  canonical MIR nodes are the carrier, flat vectors are compatibility only,
  cached display strings are diagnostics only, and the shared printer owns
  traversal while AArch64 owns rendering.

## Proof

No build needed for this markdown-only design review packet. The delegated
proof wrote concise `rg` output to `test_after.log`, verifying that
`stage2_review_layout.md` now contains a Stage 3 draft artifact map, the
mandatory directory index draft, the single mandatory `.hpp.md` draft, every
mandatory `.cpp.md` draft, the phoenix header policy, and the explicit
evidence repair dependency statement.
