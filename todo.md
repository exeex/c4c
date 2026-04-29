Status: Active
Source Idea Path: ideas/open/133_parser_namespace_visible_name_compatibility_spelling_cleanup.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Quarantine String Overloads and AST Projection Bridges

# Current Packet

## Just Finished

Plan Step 3 `Tighten Namespace and Visible-Name Context Lookup` completed.
`qualified_key_in_context(..., create_missing_path=false)` no longer repairs a
missing structured namespace path by rendering
`compatibility_namespace_name_in_context` and reparsing it as semantic
authority. The non-creating key path now returns the structured key it can
actually prove from context ids and TextIds.

`lookup_type_in_context` now gates legacy rendered typedef lookup behind the
explicit TextId-less compatibility path, matching the existing value lookup
behavior. Namespace type/value tests now cover valid-TextId rejection for
legacy rendered namespace bridges, direct qualified resolution rejection, and
the remaining explicit TextId-less compatibility branches.

## Suggested Next

Execute a bounded `plan.md` Step 4 packet against string-returning
visible-name and namespace helpers. Start by auditing `resolve_visible_*`
string overloads, `resolve_visible_*_name`, and `lookup_*_in_context` string
overloads, then convert one semantic call-site family to consume
`VisibleNameResult`, `QualifiedNameKey`, `TextId`, or namespace context ids
directly while leaving final AST spelling, diagnostics, debug output, and
explicit fallback bridges intact.

## Watchouts

The preserved compatibility path is intentionally TextId-less. Imported
namespace value compatibility already existed and remains covered; the new
type-side coverage is direct namespace and qualified resolution only, because
imported type fallback was not accepted by the current implementation.

## Proof

Ran the supervisor-selected proof:
`cmake --build build --target frontend_parser_tests > test_after.log 2>&1 && ctest --test-dir build -R '^frontend_parser_tests$|using_namespace_directive_parse|local_value_shadows_using_alias_assign_expr_parse' --output-on-failure >> test_after.log 2>&1`

Result: passed; proof log is `test_after.log`.

Supervisor accepted and committed this Step 3 slice as `ec08d028`; canonical
execution is advanced to Step 4.
