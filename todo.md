Status: Active
Source Idea Path: ideas/open/226_aarch64_module_phoenix_review_replacement_layout.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Define Replacement Architecture Layout

# Current Packet

## Just Finished

Step 3: Define Replacement Architecture Layout extended
`src/backend/mir/aarch64/module/stage2_review_layout.md` with the direct
prepared-BIR-to-MIR replacement architecture. The artifact now defines
dependency direction and responsibility seams for module dispatch, function
traversal, value/operand resolution, instruction lowering, branch/control
lowering, call lowering, public assembly bridging, and compatibility behavior,
including what each component may know, must not know, and outputs. It also
records an explicit parent-224 judgment and rejects preserving the legacy
catch-all module emitter under renamed helpers.

## Suggested Next

Delegate Step 4 draft artifact map work: name the exact mandatory replacement
`.cpp.md` and `.hpp.md` draft artifacts that Stage 3 must fill, while keeping
the header policy visible and still avoiding draft contents.

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
- Step 4 should only declare the Stage 3 draft artifact map; do not write the
  replacement draft contents yet.
- Keep the parent-224 constraints from Step 3 visible when naming draft files:
  canonical MIR nodes are the carrier, flat vectors are compatibility only,
  cached display strings are diagnostics only, and the shared printer owns
  traversal while AArch64 owns rendering.

## Proof

No build needed for this markdown-only design review packet. The delegated
proof wrote concise `rg` output to `test_after.log`, verifying that
`stage2_review_layout.md` now contains the replacement architecture section,
responsibility and dependency seams, component ownership for module dispatch,
function traversal, value/operand resolution, instruction lowering,
branch/control lowering, call lowering, public assembly bridging,
compatibility behavior, and an explicit parent-224 judgment.
