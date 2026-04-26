# BIR Raw Label Fallback Cleanup After Assembler Id Path

Status: Closed
Created: 2026-04-26
Last Updated: 2026-04-26
Closed: 2026-04-26

Parent Ideas:
- [119_bir_block_label_structured_identity_for_assembler.md](/workspaces/c4c/ideas/closed/119_bir_block_label_structured_identity_for_assembler.md)

## Goal

Remove or narrow raw BIR label string fallback dependencies after assembler and
backend consumers prove they can rely on the structured `BlockLabelId` path.

## Why This Idea Exists

Idea 119 added structured label identity beside existing raw label spellings and
kept dump text stable. That dual path is complete, but raw strings remain as
compatibility fallbacks and construction conveniences. Removing them is a
separate cleanup because assembler consumption should first prove that ids are
sufficient authority.

## Scope

Candidate cleanup surfaces:

- BIR structs still store raw labels beside ids for blocks, branch targets,
  conditional branch targets, phi incoming labels, and label-address bases.
- Validation and fixture-style construction paths can still create raw-only BIR
  until the lowering/interning boundary or explicit tests attach ids.
- CLI focus filters and matching still compare requested function, block, and
  value text against raw BIR or prepared spellings.
- Prepared liveness, stack-layout, dynamic-stack, and out-of-SSA paths still
  intern or match some block/predecessor labels from raw strings.
- Legacy prepared/raw consumers and target-local MIR/codegen routes still
  depend on raw label spelling; MIR migration should remain a separate decision
  if it grows beyond fallback cleanup.

## Execution Rules

- Do not remove raw string fallbacks until assembler/backend id consumption is
  proven for the affected path.
- Preserve existing BIR dump text unless a later supervisor explicitly accepts
  a contract change.
- Prefer converting raw-string consumers to structured ids over adding new
  string matching.
- Keep same-spelled labels across functions compatible with the module table
  model established in idea 119.
- Treat unresolved ids as proof gaps; do not fabricate ids or weaken tests to
  hide raw-only paths.

## Acceptance Criteria

- Raw fallback dependencies from idea 119 are either removed, narrowed to
  explicit compatibility boundaries, or documented as intentionally retained
  because an upstream consumer still lacks id support.
- BIR construction, printing, focus filters, prepared handoff, and assembler
  consumption have clear authority boundaries for label spelling.
- Existing dump spelling remains stable unless a contract change is approved.
- Tests cover both valid id rendering and any intentionally retained fallback
  behavior.

## Closure Summary

Closed after Step 6 final validation. Raw BIR label spelling remains only as an
explicit compatibility payload for stable dumps, raw-only fixtures,
unresolved-id paths, selector spelling boundaries, older prepared-module
contract fixtures, and target-local MIR/codegen routes that belong to separate
work. Ordinary BIR construction, printing, focus handling, prepared pipeline
consumers, and the bounded x86 prepared handoff path now prefer structured
`BlockLabelId` authority where ids are available.

Close proof used the accepted Step 5 backend baseline in `test_before.log` and
the fresh Step 6 backend proof in `test_after.log` for the same backend command
with x86 backend tests disabled by configuration. Both logs report 108/108
enabled backend tests passing; the close-time regression guard passed in
non-decreasing mode with no new failures.
