Status: Active
Source Idea Path: ideas/open/87_parser_visible_name_resolution_structured_result.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Retarget Visible Type Lookup

# Current Packet

## Just Finished

Step 3 retargeted additional visible type consumers outside the initial
carrier packet so semantic checks consume `VisibleNameResult` before rendering
compatibility spelling.

Files changed:

- `src/frontend/parser/parser_core.cpp`: retargeted alias-template info
  lookup to derive the follow-up key from `resolve_visible_type(...)` context
  and `base_text_id` instead of reparsing rendered spelling.
- `src/frontend/parser/parser_declarations.cpp`: made visible type equality
  compare structured keys when available, and made top-level using type import
  prefer `resolve_qualified_type(...)` plus `find_structured_typedef_type(...)`
  before falling back to compatibility spelling.
- `src/frontend/parser/parser_expressions.cpp`: kept typedef-cast AST spelling
  at the boundary while resolving the direct type through
  `resolve_visible_type(...)`.
- `src/frontend/parser/parser_types_base.cpp`: changed type/value
  disambiguation and template-owner follow-through to test structured type
  results instead of non-empty compatibility strings.
- `src/frontend/parser/parser_types_declarator.cpp`: retargeted dependent
  typename fallback resolution to use `resolve_visible_type(...)` and render
  only for the output spelling.
- `src/frontend/parser/parser_types_template.cpp`: moved template primary,
  specialization, and deferred NTTP owner lookup follow-through onto
  structured type result context and `base_text_id`.

## Suggested Next

Next coherent packet: move to Step 4 value/concept visible-name result
retargeting, keeping type spelling wrappers as compatibility surfaces unless a
new caller is proven to need semantic identity.

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
- Remaining `resolve_visible_type_name(...)` and
  `resolve_qualified_type_name(...)` references are the compatibility wrapper
  definitions themselves. Direct `find_visible_typedef_type(...)` callers are
  intentionally type-object consumers, and helpers such as
  `visible_type_head_name(...)` / `resolve_qualified_known_type_name(...)`
  remain string-returning AST, fallback, or bridge boundaries over structured
  lookup.

## Proof

Ran exactly:

`{ cmake --build build -j --target c4c_frontend c4cll && ctest --test-dir build -j --output-on-failure -R '^frontend_parser_tests$'; } > test_after.log 2>&1`

Result: passed. `test_after.log` contains the successful build and focused
`frontend_parser_tests` run.

Baseline candidate for commit `86211995` was accepted after the just-finished
Step 2 carrier packet.
