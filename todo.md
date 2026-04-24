Status: Active
Source Idea Path: ideas/open/87_parser_visible_name_resolution_structured_result.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Retarget Visible Value And Concept Lookup

# Current Packet

## Just Finished

Step 4 retargeted visible value and concept lookup consumers onto structured
visible-name results, while preserving the old string-wrapper fallback behavior
at compatibility boundaries.

Files changed:

- `src/frontend/parser/parser.hpp`: declared the structured visible value and
  concept resolver entry points alongside the remaining string compatibility
  wrappers.
- `src/frontend/parser/parser_core.cpp`: implemented structured value/concept
  resolution, kept unresolved structured results empty, and restored
  string-returning value/concept wrappers to fall back to the original spelling
  when lookup fails.
- `src/frontend/parser/parser_declarations.cpp`: retargeted declaration-side
  value lookup consumers to use structured results instead of direct rendered
  spelling.
- `src/frontend/parser/parser_expressions.cpp`: retargeted expression-side
  value lookup consumers to structured visible-name results.
- `src/frontend/parser/parser_types_base.cpp`: retargeted type-head value and
  concept classification onto structured results; value/type-head
  classification now checks structured function keys first, then the
  compatibility spelling fallback when a structured value result has no matching
  key.

## Suggested Next

Next coherent packet: move to Step 5 using-alias and namespace-stack publishing,
starting with `lookup_using_value_alias(...)` so value aliases can publish the
target key directly instead of forcing structured lookup to re-enter through
rendered alias spelling.

## Watchouts

- Keep source idea 87 focused on visible-name result representation, not a
  parser-wide semantic cleanup.
- Do not repair one namespace, alias, value, type, or concept spelling with a
  testcase-shaped shortcut.
- Preserve string-returning helpers as compatibility wrappers until their
  callers have been intentionally retargeted.
- Do not mix this semantic parser identity route with parser/HIR header
  hierarchy work from ideas 92 or 93.
- `lookup_using_value_alias(...)` still returns a string bridge. Step 4 marks
  using-alias value/type results as `UsingAlias` after re-entering through that
  spelling, but Step 5 should publish the alias target key directly.
- Global qualified unqualified spelling such as `::T` remains a compatibility
  fallback in the structured wrapper to preserve existing string behavior.
- Remaining `resolve_visible_type_name(...)` and
  `resolve_qualified_type_name(...)` references are the compatibility wrapper
  definitions themselves. Direct `find_visible_typedef_type(...)` callers are
  intentionally type-object consumers, and helpers such as
  `visible_type_head_name(...)` / `resolve_qualified_known_type_name(...)`
  remain string-returning AST, fallback, or bridge boundaries over structured
  lookup.
- Remaining `resolve_visible_value_name(...)`,
  `resolve_qualified_value_name(...)`, and `resolve_visible_concept_name(...)`
  references are compatibility wrapper definitions or explicit spelling
  boundaries.
- Structured `resolve_visible_value(...)` and `resolve_visible_concept(...)`
  must continue to return an empty result for unresolved names; only the
  string-returning compatibility wrappers fall back to the input spelling.
- `classify_visible_value_or_type_head(...)` now intentionally mirrors the old
  rendered-string function check only after a structured value result was found
  and its key did not match.

## Proof

Ran exactly:

`{ cmake --build build -j --target c4c_frontend c4cll && ctest --test-dir build -j --output-on-failure -R '^frontend_parser_tests$'; } > test_after.log 2>&1`

Result: passed. `test_after.log` contains the successful build and focused
`frontend_parser_tests` run.

Baseline candidate for commit `86211995` was accepted after the just-finished
Step 2 carrier packet.
