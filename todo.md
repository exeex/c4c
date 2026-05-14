Status: Active
Source Idea Path: ideas/open/226_aarch64_module_phoenix_review_replacement_layout.md
Source Plan Path: plan.md
Current Step ID: Step 2
Current Step Title: Reconstruct Current Subsystem Shape

# Current Packet

## Just Finished

Step 1: Review Stage 1 Evidence Shape repaired the per-file Stage 1 evidence
for `module.cpp.md` and `module.hpp.md`. The evidence now describes the
replacement boundary requested for Stage 2: `PreparedBirModule` lowers directly
to MIR nodes, target-owned printable instruction/operand/register types carry
assembly spelling, and the shared `mir_printer` remains a platform-independent
single-pass emitter.

## Suggested Next

Delegate Step 2 layout work using the repaired Stage 1 evidence as the
boundary: design around prepared BIR to target MIR nodes plus the shared
printer interface, rather than around the legacy module-record pile.

## Watchouts

- Preserve the Stage 1 correction: `src/backend/mir/aarch64/module/module.cpp`
  remains compiled legacy evidence until replacement implementation plus build
  wiring exist.
- Stage 2 does not own implementation edits, build rewiring, test expectation
  changes, or replacement draft contents.
- Reject a route that rubber-stamps weak extraction, preserves the legacy module
  emitter under new names, or avoids direct prepared-BIR-to-MIR machine-node
  lowering.
- The Stage 1 per-file evidence needed this repair before Stage 2 layout work;
  it is now aligned enough to use as the replacement-boundary input.

## Proof

No build needed. Read-only markdown inspection plus `rg` checks confirmed the
updated docs mention `PreparedBirModule`, direct prepared BIR/MIR-node lowering,
the shared `mir_printer`, platform-independent printer traversal, target-owned
print/render methods, and printable immediates/register operands. A separate
inspection confirmed the forbidden Python representation-method spelling was
not introduced as a C++ API name. No `test_after.log` was required by the
delegated packet.
