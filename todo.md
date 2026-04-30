# Current Packet

Status: Active
Source Idea Path: ideas/open/139_parser_sema_rendered_string_lookup_removal.md
Source Plan Path: plan.md
Current Step ID: Step 2
Current Step Title: Remove Parser Rendered-String Semantic Lookup Routes

## Just Finished

Step 2 removed the parser known-function rendered-string compatibility route
introduced/quarantined in `da66aee`. The public parser API no longer declares or
defines the `std::string` known-function overloads,
`has_known_fn_name_compatibility_fallback()`, or
`register_known_fn_name_compatibility_fallback()`. Parser-owned declaration and
type-head callers now use `QualifiedNameKey`, `TextId`, namespace context id,
or direct structured registration; no rendered-string known-function lookup
fallback remains in the parser tree.

## Suggested Next

Continue Step 2 with the next parser rendered-string semantic route, likely the
visible typedef/value lookup paths in `src/frontend/parser/impl/core.cpp` that
still compute `visible_name_spelling()` after structured resolution.

Suggested proof command for the next narrow parser packet:
`cmake --build build --target c4cll && ctest --test-dir build -R 'cpp_(positive_parser|positive_sema|negative_tests)' --output-on-failure`

## Watchouts

This packet did not expose a missing metadata blocker: operator, constructor,
ordinary declaration, and type-head known-function paths all had structured
carriers available. The next visible-typedef/value route may still expose
local-scope or using-namespace metadata gaps; if so, stop on that exact missing
carrier instead of preserving string rediscovery.

## Proof

Ran delegated proof:
`(cmake --build build --target c4cll && ctest --test-dir build -R 'cpp_(positive_parser|positive_sema|negative_tests)' --output-on-failure) > test_after.log 2>&1`

Result: passed, 926/926 tests green. Proof log: `test_after.log`.
