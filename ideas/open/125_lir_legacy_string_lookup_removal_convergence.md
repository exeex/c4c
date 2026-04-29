# LIR Legacy String Lookup Removal Convergence

Status: Open
Created: 2026-04-29

Parent Ideas:
- [105_hir_to_lir_text_id_bridge_inventory_and_cleanup.md](/workspaces/c4c/ideas/closed/105_hir_to_lir_text_id_bridge_inventory_and_cleanup.md)
- [118_lir_bir_legacy_type_text_removal.md](/workspaces/c4c/ideas/closed/118_lir_bir_legacy_type_text_removal.md)
- [122_bir_string_legacy_path_cleanup.md](/workspaces/c4c/ideas/closed/122_bir_string_legacy_path_cleanup.md)

## Goal

Converge `src/codegen/lir` away from legacy `std::string` lookup maps so LIR
acts as a text-id and semantic-id carrier between HIR and BIR rather than as a
place where identity is rediscovered from rendered spellings.

Legacy text may remain as final LLVM spelling, dump text, diagnostic text,
compatibility payload, or unresolved producer boundary. Pure text lookup should
use `TextId`; semantic lookup should use `LinkNameId`, typed LIR records, or
domain-specific semantic tables.

## Why This Idea Exists

LIR is the bridge where old rendered names and newer structured identity meet.
The recent BIR/HIR conflict showed that changing one module can reveal stale
test contracts in another. This idea narrows the LIR pass so those conflicts
can be resolved with the same rule instead of ad hoc per-test behavior.

Shared standard:

- `TextId` is text identity only; it is not semantic authority
- `std::unordered_map<std::string, T>` text lookup should become
  `std::unordered_map<TextId, T>` where the text is interned
- `LinkNameId`, struct-name IDs, typed LIR records, and domain tables are
  semantic authority when present
- rendered strings are final spelling, display, compatibility, or temporary
  unresolved data
- tests may be updated when they assert legacy lookup authority
- if removing a LIR string fallback requires upstream HIR metadata or
  downstream BIR contract work, create a separate idea instead of folding it in

## Scope

- Inventory `std::unordered_map<std::string, ...>`, symbol matching, and
  rendered-name fallback surfaces under `src/codegen/lir`.
- Convert pure text lookup maps to `TextId` keys where LIR already carries
  interned text.
- Prefer existing `LinkNameId`, struct-name IDs, typed call records, structured
  type references, and domain semantic tables where they already exist.
- Thread structured identity through direct calls, extern declarations,
  globals, function carriers, and pointer/global references where LIR currently
  still relies on rendered strings.
- Update LIR and backend route tests that protect legacy string lookup as
  authority.
- Keep final emitted LLVM spelling stable unless an output contract change is
  explicitly accepted.

## Out Of Scope

- Parser or HIR source lookup cleanup.
- BIR verifier policy changes beyond what is required to consume structured LIR
  identity.
- Broad type-ref redesign not directly tied to lookup removal.
- Rewriting MIR or target assembly behavior.
- Weakening supported tests or replacing semantic checks with string-shaped
  shortcuts.

## Conflict Policy

When LIR cleanup collides with HIR or BIR tests:

- if LIR has `TextId` for text lookup or a domain semantic carrier for semantic
  lookup, that carrier wins over drifted raw spelling
- if a test expected drifted raw spelling to remain semantic identity, update
  the test contract
- if final output spelling genuinely needs the legacy string, classify it as
  final spelling rather than lookup authority
- split new upstream/downstream blockers into new open ideas

## Acceptance Criteria

- Covered LIR pure text lookup paths use `TextId` keys.
- Covered LIR semantic lookup paths carry domain semantic identity into BIR.
- Legacy string lookup fallbacks are removed or classified by retained purpose.
- LIR/BIR focused tests prove `TextId` and semantic identity survive drifted
  rendered spelling.
- Final emitted spelling remains stable unless intentionally changed.
- Any unresolved cross-module blocker is represented by a separate idea.
