Status: Active
Source Idea Path: ideas/open/134_parser_ast_template_payload_string_bridge_cleanup.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Quarantine NTTP and Template Argument Debug Text

# Current Packet

## Just Finished

Plan Step 3 `Quarantine NTTP and Template Argument Debug Text` continued with
a parser-side canonical key slice. `canonical_template_struct_type_key` in
`types/types_helpers.hpp` now builds template argument keys from structured
`TemplateArgRef` payload first: value args use `value`, and type args use the
nested structured `TypeSpec` key. `debug_text` remains only as an explicit
fallback when the type argument has no usable structured key.

Focused coverage now includes a canonical template-struct key case where two
template argument refs have identical structured `kind/value` payloads but
different `debug_text`; they produce the same key and the key contains the
structured `v:7` value.

## Suggested Next

Continue Step 3 with parser `types/base.cpp` template-arg ref rendering or
the `$expr:` / `template_arg_nttp_names` projection path. Prefer a narrow call
site where a structured expression node, value, or `TypeSpec` already exists
and string fallback can be named explicitly.

## Watchouts

`debug_text` is still used as an explicit fallback for structurally unknown
type arguments in the canonical key helper. This packet intentionally did not
touch HIR template materialization or parser-side `$expr:` /
`template_arg_nttp_names` flows. Avoid changing broad `Node::name` or
`TypeSpec::tag` behavior while continuing Step 3.

## Proof

Ran the supervisor-selected proof:
`cmake --build build --target c4cll frontend_parser_tests > test_after.log 2>&1 && ctest --test-dir build -R '^frontend_parser_tests$|template_alias_nttp_expr|template_qualified_nttp_parse|template_type_context_nttp_parse|template_typedef_nttp_variants_parse|qualified_trait_value_template_arg_parse' --output-on-failure >> test_after.log 2>&1`

Result: passed; proof log is `test_after.log`.
