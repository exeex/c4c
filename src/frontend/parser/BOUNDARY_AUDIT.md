# Parser Boundary Audit Inventory

Last Updated: 2026-03-29
Plan Source: `ideas/open/07_parser_whitelist_boundary_audit.md`

This note is the Step 1 inventory for the active parser whitelist-boundary
audit. It records parser sites that still depend on broad start probes, generic
recovery, or parse-and-discard behavior, and tags each site for the Step 2
ranking pass.

## Constraint / `requires` Boundaries

1. `src/frontend/parser/declarations.cpp:157`
   `is_cpp20_requires_clause_decl_boundary(Parser&)` is a local declaration
   boundary whitelist that already avoids splitting after `::` and `Identifier<`.
   Tag: `acceptable breadth`

2. `src/frontend/parser/declarations.cpp:219`
   `skip_cpp20_constraint_atom(Parser&)` still consumes a generic token stream
   until a small set of hard stops appears. It tracks nesting, but any token not
   on the stop list is accepted by default.
   Tag: `known bug`
   Evidence: the active idea was triggered by reduced `iterator_concepts.h` and
   `functional_hash.h` failures, and the current reduced regression
   `tests/cpp/internal/postive_case/cpp20_requires_trait_disjunction_function_parse.cpp`
   exists because this boundary had been too broad.

3. `src/frontend/parser/declarations.cpp:308`
   `parse_optional_cpp20_requires_clause(Parser&)` now refuses empty
   declaration-level constraints except for the explicit record-declaration
   boundary check at `declarations.cpp:145`.
   Tag: `acceptable breadth`

4. `src/frontend/parser/declarations.cpp:335`
   `parse_optional_cpp20_trailing_requires_clause(Parser&)` now reuses the same
   atom-by-atom constraint walk as the ordinary `requires` parser, so it stops
   after a complete constraint-expression instead of greedily consuming the
   next declaration.
   Tag: `acceptable breadth`
   Evidence: the reduced regression
   `tests/cpp/internal/postive_case/cpp20_trailing_requires_following_member_decl_parse.cpp`
   plus the dedicated dump assertion keep the following `inner` declaration
   visible after a malformed constrained member prelude.

5. `src/frontend/parser/types.cpp:497`
   The older `types.cpp` copy of `is_cpp20_requires_clause_decl_boundary(Parser&)`
   falls back directly to `parser.is_type_start()`.
   Tag: `known bug`
   Reason: this is materially broader than the newer declaration-side helper and
   can inherit every false positive from `is_type_start()`.

6. `src/frontend/parser/types.cpp:512`
   The older `types.cpp` copy of `skip_cpp20_constraint_atom(Parser&)` shares the
   same broad consume-until-stop shape and still depends on the broader
   `types.cpp` boundary probe.
   Tag: `known bug`

7. `src/frontend/parser/types.cpp:601`
   The older `types.cpp` copy of `parse_optional_cpp20_requires_clause(Parser&)`
   still treats any consumed token run as a valid constraint expression.
   Tag: `suspicious breadth`

## Generic Recovery Loops

1. `src/frontend/parser/types.cpp:4426`
   `recover_record_member_parse_error(int)` skips until `;` or an unmatched `}`
   while only tracking brace depth.
   Tag: `known bug`
   Evidence: the source idea calls out record-member recovery swallowing to `}`
   as one of the motivating failures. This helper is the direct mechanism.
   Progress: the active runbook has now tightened two reduced cases so recovery
   can stop at the next member boundary for malformed special members and for a
   malformed non-alias `using` member, but broader malformed member shapes are
   still suspicious.

2. `src/frontend/parser/types.cpp:5872`
   `try_parse_record_member_or_recover(...)` only treats
   `SyncedAtSemicolon` as recoverable and rethrows on `StoppedAtRBrace`. That is
   narrower than the helper above, but it still depends on the broad skip path.
   Tag: `suspicious breadth`

3. `src/frontend/parser/statements.cpp:10`
   `parse_block()` uses statement-level recovery that skips to `;` or `}` after
   any exception.
   Tag: `acceptable breadth`
   Reason: this is a normal statement-boundary recovery rule, but it should stay
   out of declaration parsing decisions.

## Start-Condition Probes

1. `src/frontend/parser/types.cpp:2560`
   `Parser::is_type_start()` intentionally accepts many starters, including
   qualifiers, storage-class specifiers, unresolved typedef-like identifiers,
   `Identifier<`, and qualified names.
   Tag: `suspicious breadth`
   Reason: this is necessary in some contexts, but it is too broad to reuse as a
   declaration-boundary detector inside `requires` parsing.

2. `src/frontend/parser/types.cpp:2633`
   `Parser::can_start_parameter_type()` is even broader than `is_type_start()`
   because it also accepts unresolved identifier forms used during parameter
   recovery.
   Tag: `acceptable breadth`
   Reason: broadness is expected here, but callers should treat it as a local
   parameter probe only.

3. `src/frontend/parser/statements.cpp:279`
   `parse_stmt()` uses `is_type_start()` for `for (...)` initializer
   disambiguation.
   Tag: `acceptable breadth`

4. `src/frontend/parser/statements.cpp:620`
   `parse_stmt()` uses `is_type_start()` again to choose declaration vs
   expression parsing for local statements, with a special qualified-call guard.
   Tag: `acceptable breadth`

## `NK_EMPTY` Parse-And-Discard Sites

1. `src/frontend/parser/statements.cpp:77`
   Local `using` aliases and `using namespace` forms used to skip directly to
   `;` and return `NK_EMPTY`; the current tightening pass now parses them
   structurally enough to recover malformed local `using` forms as
   `NK_INVALID_STMT` at the next local boundary.
   Tag: `acceptable breadth`
   Evidence: the reduced regression
   `tests/cpp/internal/parse_only_case/local_using_alias_recovery_preserves_following_decl_parse.cpp`
   keeps the following `kept` declaration visible after a malformed local alias.

2. `src/frontend/parser/declarations.cpp:510`
   Top-level `extern "C"` parsing now drops empty linkage-spec blocks instead of
   materializing a synthetic `NK_EMPTY` node when the body does not produce
   declarations.
   Tag: `acceptable breadth`
   Evidence: the reduced parse-only regression
   `tests/cpp/internal/parse_only_case/top_level_extern_c_empty_block_preserves_following_decl_parse.cpp`
   keeps the following `kept` global visible without an intermediate `Empty`
   node after `extern "C" {}`.

3. `src/frontend/parser/declarations.cpp:1783`
   The malformed top-level storage-class fallback used to skip blindly to `;`
   when no type followed `static` / `extern` / related specifiers.
   Tag: `acceptable breadth`
   Evidence: the reduced parse-only regression
   `tests/cpp/internal/parse_only_case/top_level_storage_class_recovery_preserves_following_decl_parse.cpp`
   now proves `extern foo` stops before the next-line `int kept;` declaration
   instead of erasing it through broad `skip_until(';')` recovery.

4. `src/frontend/parser/declarations.cpp:1925`
   The generic top-level no-type-start recovery now reuses the shared
   declaration-boundary helper, so malformed discarded declarations stop before
   later `class` / `namespace` / `constexpr` / `consteval` starters instead of
   swallowing them.
   Tag: `acceptable breadth`
   Evidence: the reduced parse-only regression
   `tests/cpp/internal/parse_only_case/top_level_class_recovery_preserves_following_decl_parse.cpp`
   keeps the following `class kept {};` definition visible after a malformed
   top-level `friend` line.

5. `src/frontend/parser/declarations.cpp:1336`
   Top-level `asm(...)` parsing now drops valid declarations out of the program
   item stream and recovers before the next strong declaration starter when the
   parenthesized payload never closes, instead of materializing a synthetic
   `NK_EMPTY` node or swallowing the following declaration through a blind
   group skip.
   Tag: `acceptable breadth`
   Evidence: the reduced parse-only regressions
   `tests/cpp/internal/parse_only_case/top_level_asm_decl_preserves_following_decl_parse.cpp`
   and
   `tests/cpp/internal/parse_only_case/top_level_asm_recovery_preserves_following_decl_parse.cpp`
   keep the following `kept` global visible with no intermediate `Empty` node
   after valid `asm("nop");` input and after malformed `asm(` recovery.

6. `src/frontend/parser/declarations.cpp:924`
   Top-level `using namespace`, `using ns::name`, and `using Alias = type`
   handling now performs its namespace / alias bookkeeping and then drops out of
   the item stream instead of materializing synthetic `NK_EMPTY` nodes, while
   still using the shared declaration-boundary helpers when recovery is needed.
   Tag: `acceptable breadth`
   Evidence: the reduced parse-only regression
   `tests/cpp/internal/parse_only_case/top_level_using_decl_preserves_following_decl_parse.cpp`
   keeps the following `kept` global visible with no intermediate `Empty` node
   after valid `using` declarations, while the existing recovery regressions
   `top_level_using_namespace_recovery_preserves_following_decl_parse.cpp` and
   `top_level_using_alias_recovery_preserves_following_decl_parse.cpp` still
   prove malformed missing-semicolon cases preserve the following declaration.

8. `src/frontend/parser/declarations.cpp:911`
   Empty top-level `namespace {}` / `namespace ns {}` wrappers now drop out of
   the AST entirely instead of materializing a synthetic `NK_EMPTY` node when
   the body contributes no declarations.
   Tag: `acceptable breadth`
   Evidence: the reduced parse-only regression
   `tests/cpp/internal/parse_only_case/top_level_empty_namespace_block_preserves_following_decl_parse.cpp`
   keeps the following `kept` global visible without an intermediate `Empty`
   node after an empty namespace block.

9. `src/frontend/parser/declarations.cpp:2086`
   Top-level tag-only declarations and typedef-backed tag definitions now drop
   out of the item stream when the real `StructDef` / `EnumDef` has already
   been recorded in the parser tag-definition tables, instead of appending a
   synthetic `NK_EMPTY` node after `struct X {};`, `union Y {};`, or
   `typedef enum Z { ... } Z;`.
   Tag: `acceptable breadth`
   Evidence: the reduced parse-only regression
   `tests/cpp/internal/parse_only_case/top_level_tag_decl_preserves_following_decl_parse.cpp`
   keeps the following `kept` global visible with no intermediate `Empty`
   node after structure-only and typedef-backed tag declarations.

10. `src/frontend/parser/declarations.cpp:1417`
   Top-level C++ `concept` declarations now drop out of the item stream after
   registering the concept name, instead of materializing a synthetic
   `NK_EMPTY` node for either plain or templated `concept` declarations.
   Tag: `acceptable breadth`
   Evidence: the reduced parse-only regression
   `tests/cpp/internal/parse_only_case/top_level_concept_decl_preserves_following_decl_parse.cpp`
   keeps the following `kept` global visible with no intermediate `Empty`
   node after `template<typename T> concept audit_concept = true;`.

11. `src/frontend/parser/declarations.cpp:2026`
   Top-level `typedef` declarations now drop out of the item stream after
   updating alias metadata instead of materializing a synthetic `NK_EMPTY`
   node for bookkeeping-only aliases such as `typedef int Value;`.
   Tag: `acceptable breadth`
   Evidence: the reduced parse-only regression
   `tests/cpp/internal/parse_only_case/top_level_typedef_decl_preserves_following_decl_parse.cpp`
   keeps the following `kept` global visible with no intermediate `Empty`
   node after simple and function-pointer typedef declarations.

12. `src/frontend/parser/declarations.cpp:1422`
   Top-level `#pragma pack(...)` directives now update parser pack state and
   drop out of the item stream instead of materializing a synthetic `NK_EMPTY`
   node before the following declaration.
   Tag: `acceptable breadth`
   Evidence: the reduced parse-only regression
   `tests/cpp/internal/parse_only_case/top_level_pragma_pack_preserves_following_decl_parse.cpp`
   keeps the following `kept` global visible with no intermediate `Empty`
   node after `#pragma pack(push, 1)`.

13. `src/frontend/parser/declarations.cpp:1437`
   Top-level `#pragma GCC visibility push/pop` directives now update parser
   visibility state and drop out of the item stream instead of materializing a
   synthetic `NK_EMPTY` node before the following declaration.
   Tag: `acceptable breadth`
   Evidence: the reduced parse-only regression
   `tests/cpp/internal/parse_only_case/top_level_pragma_gcc_visibility_preserves_following_decl_parse.cpp`
   keeps the following `kept` global visible with no intermediate `Empty`
   node after `#pragma GCC visibility push(hidden)`.

## Ranked First Tightening Targets

1. `src/frontend/parser/types.cpp:4426`
   Narrow record-member recovery so malformed members cannot silently consume up
   to the record-closing `}` without a more explicit outcome.

2. `src/frontend/parser/declarations.cpp:1444`
   Review the remaining top-level structure-only / unsupported `NK_EMPTY` exits,
   starting with valid top-level `asm(...)` declarations that still leave a
   synthetic `Empty` node in the program item stream.
