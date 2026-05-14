Status: Active
Source Idea Path: ideas/open/226_aarch64_module_phoenix_review_replacement_layout.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Reconstruct Current Subsystem Shape

# Current Packet

## Just Finished

Step 2: Reconstruct Current Subsystem Shape added
`src/backend/mir/aarch64/module/stage2_review_layout.md`. The artifact maps
the current entry point, prepared BIR inputs, prepared authority records,
codegen records, public assembly/API exposure, hidden cross-helper state, and
special-case classifications without adopting the legacy helper boundaries as
replacement design.

## Suggested Next

Delegate Step 3 replacement layout work: define the direct prepared-BIR-to-MIR
machine-node lowering architecture from the current-shape evidence, including
responsibility ownership and dependency direction, without drafting per-file
replacement contents yet.

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
- Step 3 still needs to judge parent 224 explicitly and define the replacement
  architecture; Step 2 only reconstructs the current subsystem shape.

## Proof

No build needed for this markdown-only design review packet. The delegated
proof wrote concise `rg` output to `test_after.log`, verifying that
`stage2_review_layout.md` exists and contains sections for current entry
points, prepared BIR inputs, prepared authority records, codegen records,
public assembly/API exposure, hidden cross-helper state, and special-case
classification.
