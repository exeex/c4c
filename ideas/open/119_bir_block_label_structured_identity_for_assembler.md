# BIR Block Label Structured Identity For Assembler

Status: Open
Created: 2026-04-26
Last Updated: 2026-04-26

Parent Ideas:
- [116_bir_layout_dual_path_coverage_and_dump_guard.md](/workspaces/c4c/ideas/closed/116_bir_layout_dual_path_coverage_and_dump_guard.md)
- [117_bir_printer_structured_render_context.md](/workspaces/c4c/ideas/open/117_bir_printer_structured_render_context.md)

## Goal

Move BIR block labels from raw rendered strings toward shared structured label
identity so later assembler and backend stages can reference labels by id while
still rendering stable `xxxx:` dump text.

## Why This Idea Exists

BIR currently stores block labels and branch targets as strings:

- `bir::Block::label`
- `bir::PhiIncoming::label`
- `bir::Terminator::target_label`
- `bir::Terminator::true_label`
- `bir::Terminator::false_label`
- `bir::MemoryAddress::BaseKind::Label` plus `base_name`

That is not a blocker for LIR/BIR type legacy removal because labels are CFG
spelling, not type/layout authority. It is still a real structured-identity
gap for the assembler route: an assembler should be able to consume stable
label ids and look up the final spelling from a shared table instead of
depending on repeated raw strings.

`src/shared/text_id_table.hpp` already defines `BlockLabelId`,
`kInvalidBlockLabel`, and `BlockLabelTable` as a `TextId`-backed
`SemanticNameTable`. This idea should reuse that existing shared table unless
execution proves a separate `LabelNameId` alias is clearer for assembler
terminology.

## Scope

Candidate work includes:

- attach a shared `TextTable` / `BlockLabelTable` path to BIR label creation
- add `BlockLabelId` fields beside existing rendered string labels in BIR
  blocks, phi incoming edges, branch terminators, and label-address operands
- keep raw string labels as compatibility/render fallback during the dual path
- make label rendering prefer `BlockLabelId -> TextId -> spelling` where
  available
- validate branch, conditional branch, phi incoming, focused dump filters, and
  label-address uses still render the same text
- record assembler-facing handoff notes for later MIR/assembler consumption

Out of scope:

- Removing raw label strings immediately.
- Reworking type/layout legacy removal from ideas 116 through 118.
- Changing final label spelling conventions.
- Migrating MIR target codegen internals.

## Execution Rules

- Preserve existing BIR dump text, including the trailing `:` on block labels.
- Do not make label identity a prerequisite for type legacy removal.
- Intern label spelling through the shared text table once per module/function
  path rather than repeatedly treating every edge string as independent
  authority.
- Keep branch target ids aligned with the corresponding block label id.
- If a label target cannot be resolved to an id, keep the raw string fallback
  and report it as a proof gap rather than silently fabricating an id.

## Acceptance Criteria

- BIR blocks and branch/phi label references can carry structured label ids
  backed by shared `TextId` spelling.
- `bir_printer.cpp` can render labels through the structured label table when
  ids are available, while preserving existing dump text.
- Focused `--dump-bir` tests for branch, conditional branch, phi incoming,
  focus-block filtering, and focus-value filtering continue to pass.
- Remaining raw-string-only label paths are inventoried for a later removal
  idea.
- The idea explicitly leaves LIR/BIR type legacy removal unblocked.

## Deferred

- Removing raw BIR label string fallbacks should be a later cleanup after
  assembler consumption proves the id path.
