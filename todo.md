Status: Active
Source Idea Path: ideas/open/133_parser_namespace_visible_name_compatibility_spelling_cleanup.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Tighten Namespace and Visible-Name Context Lookup

# Current Packet

## Just Finished

Plan Step 2 `Make Using-Value Alias Lookup Structured-Primary` completed.
`lookup_using_value_alias(..., VisibleNameResult*)` now treats a non-empty
`UsingValueAlias::compatibility_name` as authority only for explicit no-key
aliases. Structured aliases still populate display spelling from
`render_value_binding_name`, but they only resolve through
`has_known_fn_name(alias.target_key)` or
`find_structured_var_type(alias.target_key)`.

Focused parser tests now cover structured-target-missing rejection with a
non-empty compatibility bridge, explicit no-key fallback preservation, and the
type/template projection cases that intentionally pass through a known
structured target key rather than compatibility spelling.

## Suggested Next

Execute a bounded `plan.md` Step 3 packet against namespace
rendering/projection bridges, especially
`compatibility_namespace_name_in_context`, `bridge_name_in_context`,
`lookup_*_in_context`, and `resolve_namespace_*`. Keep TextId, namespace
context ids, `find_named_namespace_child`, and `qualified_key_in_context` as
primary authority; preserve TextId-less or keyless branches only as explicit,
tested compatibility fallback.

## Watchouts

Some visible type/template tests use using-value aliases as projection bridges.
Those tests now register the structured target key as known before expecting the
projection to succeed; do not reintroduce compatibility-name success for
structured aliases to satisfy similar cases.

## Proof

Ran the supervisor-selected proof:
`cmake --build build --target frontend_parser_tests > test_after.log 2>&1 && ctest --test-dir build -R '^frontend_parser_tests$' --output-on-failure >> test_after.log 2>&1`

Result: passed; proof log is `test_after.log`.

Also ran targeted parse-only selectors:
`ctest --test-dir build -R 'using_namespace_directive_parse|local_value_shadows_using_alias_assign_expr_parse' --output-on-failure`

Result: passed; both selectors were present in this build.

Supervisor accepted the baseline review for Step 2, so canonical execution is
advanced to Step 3.
