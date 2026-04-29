# HIR Legacy String Lookup Removal Convergence

Status: Open
Created: 2026-04-29

Parent Ideas:
- [99_hir_module_symbol_structured_lookup_mirror.md](/workspaces/c4c/ideas/closed/99_hir_module_symbol_structured_lookup_mirror.md)
- [122_bir_string_legacy_path_cleanup.md](/workspaces/c4c/ideas/closed/122_bir_string_legacy_path_cleanup.md)

## Goal

Converge `src/frontend/hir` away from legacy `std::string` lookup maps after
the first structured module-symbol mirror work. HIR-owned text lookup should
use `TextId` / `TextTable` keys, while semantic lookup should use declaration
IDs, domain-specific semantic tables, or typed keys rather than rendered names.

Rendered strings remain valid for diagnostics, dumps, mangled/link-visible
output, and compatibility bridges. They should not be the primary semantic
lookup key where HIR already has stable declaration metadata.

## Why This Idea Exists

HIR sits between parser/sema identity and LIR/BIR emission, so it is the module
most likely to expose mixed-contract drift. Recent backend cleanup showed that
tests may preserve old legacy behavior after production policy changes. This
idea explicitly allows test contract updates when they are enforcing legacy
string authority rather than supported semantics.

Use the shared standard:

- `TextId` is text identity only; it is not semantic authority
- `std::unordered_map<std::string, T>` text lookup should become
  `std::unordered_map<TextId, T>` where the text is interned
- structured declaration IDs, namespace context IDs, `LinkNameId`, and other
  domain tables are semantic authority when present
- rendered strings are spelling or compatibility data
- conflict between HIR and downstream tests should be resolved by the intended
  authority contract, not by preserving whichever test fails first
- separable blockers should become new ideas instead of expanding this route

## Scope

- Inventory HIR-owned `std::unordered_map<std::string, ...>`, string lookup,
  and fallback paths in `src/frontend/hir`.
- Convert pure text lookup maps to `TextId` keys where HIR already has interned
  text.
- Demote rendered-name semantic lookups for functions and globals where
  declaration IDs, semantic mirrors, or domain tables already cover the path.
- Extend structured lookup proof for namespace-qualified, template-related,
  consteval/bodyless, and link-name-backed symbol paths only where metadata is
  already available.
- Update HIR tests that still assert legacy rendered-name precedence when the
  intended semantic authority is structured identity.
- Keep rendered names available for codegen, diagnostics, dumps, mangling, and
  ABI/link spelling.

## Out Of Scope

- Parser helper cleanup.
- LIR type-ref authority redesign.
- BIR verifier/printer contract changes.
- Broad struct/type identity removal unless a focused sub-idea is created.
- Removing emitted spellings that are required for ABI, diagnostics, or dumps.

## Conflict Policy

Cross-module failures are expected during this cleanup. When they happen:

- update tests that still protect legacy `std::string` lookup as text or
  semantic authority
- preserve behavior only when the string is display/final spelling, diagnostics,
  or compatibility data, not lookup authority
- create a separate idea if a missing upstream metadata propagation step blocks
  HIR demotion

## Acceptance Criteria

- Covered HIR pure text lookups use `TextId` keys.
- Covered HIR semantic lookups use declaration IDs, domain semantic tables, or
  typed keys before rendered-name fallback.
- Remaining HIR string lookups are classified by purpose.
- Focused HIR tests prove drifted rendered names do not override covered
  `TextId` or semantic-table lookup paths.
- Downstream LIR/BIR expectations are aligned with the same authority rule or
  split into follow-up ideas.
