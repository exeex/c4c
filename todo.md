Status: Active
Source Idea Path: ideas/open/133_parser_namespace_visible_name_compatibility_spelling_cleanup.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Quarantine String Overloads and AST Projection Bridges

# Current Packet

## Just Finished

Plan Step 4 `Quarantine String Overloads and AST Projection Bridges` first
bounded slice completed. `parse_base_type` no longer calls the raw string-only
`resolve_visible_type(name)` at simple typedef parsing or deferred alias-member
owner lookup sites where a token or parser-owned TextId is available. Both
sites now call the TextId-aware overload and keep `visible_name_spelling` only
as the final projection spelling.

The owned type/template files now have no one-argument `resolve_visible_type`
calls; remaining `resolve_visible_type` use in the owned files passes TextId
identity plus fallback spelling.

## Suggested Next

Continue Step 4 by auditing other raw string typedef/template lookup bridges,
especially `find_visible_typedef_type(const char*/std::string)` and final AST
projection paths where a structured key or TextId is already available.

## Watchouts

This slice is intentionally mechanical and semantic-call-site focused; it does
not remove string overloads globally. Existing frontend parser coverage proved
the changed type/template paths without adding new test contracts.

## Proof

Ran the supervisor-selected proof:
`cmake --build build --target frontend_parser_tests > test_after.log 2>&1 && ctest --test-dir build -R '^frontend_parser_tests$|using_namespace_directive_parse|local_value_shadows_using_alias_assign_expr_parse' --output-on-failure >> test_after.log 2>&1`

Result: passed; proof log is `test_after.log`.
