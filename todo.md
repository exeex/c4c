# Current Packet

Status: Active
Source Idea Path: ideas/open/147_rendered_qualified_compatibility_bridge_removal.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Inventory Reachable Rendered-Spelling Bridges

## Just Finished

Completed Step 1 inventory of reachable rendered qualified-name compatibility
bridges.

Occurrences and classifications:

- `src/frontend/parser/impl/core.cpp:365`, `:434`
  `find_compatibility_key_from_rendered_qualified_spelling()` is a live
  compatibility bridge. It is reachable only through
  `qualified_key_in_context()` when a caller supplies a rendered `TextId`
  containing `::`. Classification: structured-carrier migration work, not
  display-only and not deletion-ready.
- `src/frontend/parser/impl/core.cpp:367`, `:477`
  `intern_compatibility_key_from_rendered_qualified_spelling()` is the mutating
  companion for the same route, also called from `:480` after a find miss.
  Classification: structured-carrier migration work.
- `src/frontend/parser/impl/core.cpp:370-380`
  `qualified_key_in_context()` is the bridge dispatcher. Its rendered `TextId`
  branch calls the compatibility helpers whenever `parser_text(name_text_id)`
  contains `::`; otherwise it builds a structured key from context and an
  unqualified base `TextId`. Classification: semantic lookup authority for the
  rendered route and first migration target.
- `qualified_key_in_context()` caller families:
  `render_lookup_name_in_context()` at `src/frontend/parser/impl/core.cpp:310`
  is display/compatibility spelling; structured typedef/value registration at
  `:1321`, `:1398`, `:1418`, and known function/concept registration at
  `:1654`, `:1685` guard against qualified rendered `TextId`s and are
  structured authority; key query helpers at `:1427`, `:1433`, `:1440`,
  `:1591`, and `:1596` also guard or intentionally key semantic maps;
  `record_member_typedef_key_in_context()` at `:1496`,
  `alias_template_key_in_context()` at `:1596`, and visible value fallback at
  `:3128` can still intern or find through the rendered branch if their
  callers pass a qualified rendered `TextId`. Classification:
  `record_member_typedef_key_in_context()` and `alias_template_key_in_context()`
  are structured-carrier migration work; visible value fallback is semantic
  lookup authority but already rejects qualified input at the public
  `lookup_value_in_context()` entry.
- Reachable caller families feeding the migration-sensitive helpers include
  alias/template lookup and registration in
  `src/frontend/parser/impl/declarations.cpp`, `types/struct.cpp`,
  `types/template.cpp`, `types/declarator.cpp`, `types/base.cpp`, plus
  expression alias lookup in `src/frontend/parser/impl/expressions.cpp`.
  These are the main places to preserve `QualifiedNameRef`/`QualifiedNameKey`
  instead of reconstituting owner/base from one rendered spelling.
- `src/shared/qualified_name_table.hpp:229`
  `split_qualified_name_scope()` and `:251`
  `qualified_name_base_matches()` have no callers found under `src/` or
  `tests/`; `has_qualified_name_scope()` at `:222` is likewise definition-only.
  Classification: dead/unreachable for this route; deletion/isolation can wait
  until semantic bridge callers are migrated.
- Sema comments/routes: `src/frontend/sema/type_utils.cpp:542`
  `same_rendered_type_name_compatibility()` is explicitly last-resort equality
  for carriers without structured metadata; `src/frontend/sema/validate.cpp:1527`
  `structured_record_key_for_type()` uses rendered tag fallback only after
  structured metadata is absent. Classification: display-only compatibility
  fallback, not first semantic lookup authority.
- HIR comments/routes: `src/frontend/hir/hir_functions.cpp:40`
  no-usable-text-metadata legacy template binding helper is in an unintegrated
  draft file and returns empty; classification: dead/unreachable draft
  compatibility note. `src/frontend/parser/ast.hpp:123` documents
  `qualifier_text_ids` as structured metadata and `qualifier_segments` as
  compatibility/display spelling; classification: structured-carrier migration
  contract.

Recommended first implementation packet: migrate one parser alias/template
caller family that currently feeds `alias_template_key_in_context()` or
`record_member_typedef_key_in_context()` from a rendered qualified `TextId` to
a carried `QualifiedNameRef`/`QualifiedNameKey`. This attacks the only live
semantic bridge entry before touching Sema/HIR comments or dead shared helpers,
and avoids claiming progress through helper deletion while parser callers can
still reconstruct semantics from `A::B` text.

## Suggested Next

Implement the first Step 2 parser packet: choose one alias/template caller
family feeding `alias_template_key_in_context()` or
`record_member_typedef_key_in_context()`, preserve structured qualified
metadata through that path, and prove the caller no longer needs a rendered
qualified `TextId`.

## Watchouts

- Do not treat helper renames or expectation rewrites as bridge-removal
  progress.
- Do not weaken qualified template or HIR tests to make the bridge deletion
  pass.
- Keep source-idea changes out of routine execution unless durable intent truly
  changes.
- Deleting `split_qualified_name_scope()` now would be low-value cleanup only;
  no reachable semantic caller was found, while `qualified_key_in_context()`
  remains live.
- Sema/HIR fallback comments are not the first packet unless paired with a real
  structured carrier migration; comment removal alone would overstate progress.

## Proof

Lifecycle/inventory-only; no build required and no `test_after.log` produced.
Commands run for confidence:
`rg -n "find_compatibility_key_from_rendered_qualified_spelling|intern_compatibility_key_from_rendered_qualified_spelling|qualified_key_in_context|split_qualified_name_scope|qualified_name_base_matches" src tests docs -g'!*build*'`;
`rg -n "structured metadata|structured-metadata|no structured|missing structured|without structured|lacks structured|fallback.*structured|structured.*fallback|rendered.*qualified|qualified.*rendered" src/frontend -g'!*build*'`;
`rg -n "split_qualified_name_scope\\(|qualified_name_base_matches\\(|has_qualified_name_scope\\(" src tests -g'!*build*'`;
`rg -n "qualified_key_in_context\\(" src/frontend/parser -g'!*build*'`.
