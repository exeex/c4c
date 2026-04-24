Status: Active
Source Idea Path: ideas/open/87_parser_visible_name_resolution_structured_result.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Narrow Using-Alias And Namespace-Stack Publishing

# Current Packet

## Just Finished

Step 5 narrowed using-alias publishing so value-alias lookup can return a
structured `VisibleNameResult` carrying the target `QualifiedNameKey` directly,
with rendered spelling produced by the compatibility string overload.

Files changed:

- `src/frontend/parser/parser.hpp`: added the structured
  `lookup_using_value_alias(...)` overload beside the remaining string
  compatibility overload.
- `src/frontend/parser/parser_core.cpp`: retargeted visible and qualified value
  alias consumers to use the structured alias result directly, and layered the
  string-returning alias helper over `visible_name_spelling(...)`.

## Suggested Next

Next coherent packet: move to Step 6 coverage and closure decision, reviewing
the remaining explicit bridge/fallback call sites before deciding whether the
source idea is complete or needs a follow-on initiative.

## Watchouts

- Keep source idea 87 focused on visible-name result representation, not a
  parser-wide semantic cleanup.
- Do not repair one namespace, alias, value, type, or concept spelling with a
  testcase-shaped shortcut.
- Preserve string-returning helpers as compatibility wrappers until their
  callers have been intentionally retargeted.
- Do not mix this semantic parser identity route with parser/HIR header
  hierarchy work from ideas 92 or 93.
- The value-alias table still has a string compatibility overload because
  visible type lookup has a legacy bridge path through value aliases. This
  packet did not broaden that table into a type/value/concept alias registry.
- Namespace-stack lookup still renders at explicit table boundaries such as
  `has_var_type(...)` and `has_typedef_type(...)`, where the backing table is
  string-keyed. Structured key checks now run first on the value-alias path.
- Global qualified unqualified spelling such as `::T` remains a compatibility
  fallback in the structured wrapper to preserve existing string behavior.

## Proof

Ran exactly:

`{ cmake --build build -j --target c4c_frontend c4cll && ctest --test-dir build -j --output-on-failure -R '^frontend_parser_tests$'; } > test_after.log 2>&1`

Result: passed. `test_after.log` contains the successful build and focused
`frontend_parser_tests` run.

Baseline candidate for commit `86211995` was accepted after the just-finished
Step 2 carrier packet.
