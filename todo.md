# Current Packet

Status: Active
Source Idea Path: ideas/open/139_parser_sema_rendered_string_lookup_removal.md
Source Plan Path: plan.md
Current Step ID: Step 2
Current Step Title: Remove Parser Rendered-String Semantic Lookup Routes

## Just Finished

Step 2 removed the next parser semantic lookup overload family under the
stricter idea 139 standard. The public parser typedef/value lookup and binding
routes for this slice now use `TextId` or `QualifiedNameKey`, not
`std::string`, `std::string_view`, or `TextId` plus fallback spelling
parameters. Covered APIs include `has_typedef_name`, `has_typedef_type`,
`find_typedef_type`, visible typedef lookup, typedef registration/unregister,
`find_var_type`, visible value lookup, and value-type registration.

This builds on the previous removal of the known-function rendered-string
compatibility route introduced/quarantined in `da66aee`. No metadata blocker was
identified for this parser API slice.

## Suggested Next

Continue Step 2 with parser API removal under the stricter standard. Start with
the next parser semantic lookup route that still exposes `std::string`,
`std::string_view`, fallback spelling, parallel string/structured overloads, or
`fallback`/`legacy` compatibility naming. Remaining likely candidates include
template/type parsing helper routes and then Sema consteval/validate lookup
families.

Suggested proof command for the next narrow parser packet:
`cmake --build build --target c4cll && ctest --test-dir build -R 'cpp_(positive_parser|positive_sema|negative_tests)' --output-on-failure`

## Watchouts

Do not retain semantic compatibility APIs with string/string_view parameters,
fallback spelling, or `fallback`/`legacy` names. Preserve strings only for
diagnostics, display, source spelling, dumps, ABI/link spelling, or final
output.

## Proof

Ran delegated proof:
`(cmake --build build --target c4cll && ctest --test-dir build -R 'cpp_(positive_parser|positive_sema|negative_tests)' --output-on-failure) > test_after.log 2>&1`

Result: passed, 926/926 tests green. Proof log: `test_after.log`.
Regression guard against `test_before.log` passed with
`--allow-non-decreasing-passed`: before 926/926, after 926/926, no new
failures.
