Status: Active
Source Idea Path: ideas/open/87_parser_visible_name_resolution_structured_result.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Introduce Structured Visible Resolution Carrier
# Current Packet

## Just Finished

Step 2 introduced the structured visible-name carrier and first type-only
structured resolver path.

Files changed:

- `src/frontend/parser/parser.hpp`: added `VisibleNameKind`,
  `VisibleNameSource`, `VisibleNameResult`, structured type resolver
  declarations, a structured `lookup_type_in_context` overload, and
  `visible_name_spelling(...)` for compatibility rendering.
- `src/frontend/parser/parser_core.cpp`: implemented
  `resolve_visible_type(...)`, `resolve_qualified_type(...)`, structured
  `lookup_type_in_context(...)`, compatibility wrappers for
  `resolve_visible_type_name(...)`, `resolve_qualified_type_name(...)`, and
  string-returning `lookup_type_in_context(...)`; retargeted
  `find_visible_typedef_type(...)` to consume the structured result first.
- `src/frontend/parser/types_helpers.hpp`: moved direct type probes in
  `visible_type_head_name(...)`, `resolve_qualified_typedef_name(...)`, and
  `resolve_qualified_known_type_name(...)` to the structured type resolver
  where practical while preserving rendered compatibility spelling at the
  existing string boundaries.

## Suggested Next

Next coherent packet: retarget additional visible type consumers outside the
initial `parser_core.cpp` / `types_helpers.hpp` path, especially
alias-template, declarator, struct/type, and expression disambiguation callers
that still request rendered type names directly before semantic checks.

## Watchouts

- Keep source idea 87 focused on visible-name result representation, not a
  parser-wide semantic cleanup.
- Do not repair one namespace, alias, value, type, or concept spelling with a
  testcase-shaped shortcut.
- Preserve string-returning helpers as compatibility wrappers until their
  callers have been intentionally retargeted.
- Do not mix this semantic parser identity route with parser/HIR header
  hierarchy work from ideas 92 or 93.
- `lookup_using_value_alias(...)` still returns a string bridge. The type
  structured resolver preserves current behavior and marks this as
  `UsingAlias`, but a later packet should publish the alias target key directly.
- Global qualified unqualified spelling such as `::T` remains a compatibility
  fallback in the structured wrapper to preserve existing string behavior.

## Proof

Ran exactly:

`{ cmake --build build -j --target c4c_frontend c4cll && ctest --test-dir build -j --output-on-failure -R '^frontend_parser_tests$'; } > test_after.log 2>&1`

Result: passed. `test_after.log` contains the successful build and focused
`frontend_parser_tests` run.
