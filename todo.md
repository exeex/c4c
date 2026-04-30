Status: Active
Source Idea Path: ideas/open/137_parser_known_function_name_compatibility_spelling_cleanup.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Map Structured and Rendered Known-Name Paths

# Current Packet

## Just Finished

Completed Plan Step 1 classification for parser known-function-name paths.
`known_fn_names` is already stored as `QualifiedNameKey`, and structured lookup
exists through `has_known_fn_name(const QualifiedNameKey&)`,
`known_fn_name_key_in_context()`, `qualified_name_key()`, `resolve_qualified_value()`,
and using-alias `VisibleNameResult` keys.

Remaining rendered-spelling authority paths:
- `Parser::register_known_fn_name(const std::string&)` reparses rendered names
  through `intern_known_fn_name_key_from_spelling()`.
- `Parser::register_known_fn_name_in_context()` falls back to
  `register_known_fn_name(std::string(fallback_name))` when `fallback_name`
  contains `::`.
- Declaration registration still sends rendered names for out-of-class operator
  declarations via `qualified_op_name`, out-of-class constructors via
  `qualified_ctor_name`, and scoped declarations via `scoped_decl_name` when
  `decl_name` already contains `::`.
- Type/expression disambiguation still lets rendered spellings decide known
  function identity in `classify_visible_value_or_type_head()` through
  `has_known_fn_name(head_name)`, `current_member_name`, and
  `visible_name_spelling(resolved_value)`.

Compatibility-only paths:
- Public string overload declarations are already documented as compatibility
  bridges in `parser.hpp`.
- `find_known_fn_name_key_from_spelling()` and
  `intern_known_fn_name_key_from_spelling()` are compatibility parsing bridges
  for string overloads and textless/fallback spellings.
- String-output visible-name overloads are projection bridges over structured
  `VisibleNameResult` lookup and should remain display/fallback-only.

## Suggested Next

Step 2 first packet: convert declaration-side registration for the common
`register_decl_known_fn_name` lambda in `declarations.cpp` so scoped and
namespace-context declarations register a `QualifiedNameKey`/context-derived key
first, leaving the rendered `scoped_decl_name` path only as an explicit
compatibility fallback.

## Watchouts

- Keep source-idea intent stable; routine findings belong in this file.
- Do not let rendered spelling decide disambiguation when structured identity
  is available.
- Avoid testcase-shaped special cases.
- Treat out-of-class operator and constructor registration as separate follow-up
  packets; they need owner-name structure preserved instead of reparsing
  `qualified_op_name`/`qualified_ctor_name`.
- Do not remove `fn->name` rendered spelling yet; it is final AST/display data,
  not the known-name authority store.

## Proof

No build run; packet was classification-only per delegation. Sanity proof used
`c4c-clang-tool-ccdb` signature/type-reference queries for `core.cpp`,
`declarations.cpp`, and `types/base.cpp`, plus narrow source windows around the
identified call sites. No `test_after.log` was generated because no build/test
proof was required for this packet.
