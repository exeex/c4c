Status: Active
Source Idea Path: ideas/open/134_parser_ast_template_payload_string_bridge_cleanup.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Quarantine NTTP and Template Argument Debug Text

# Current Packet

## Just Finished

Plan Step 3 `Quarantine NTTP and Template Argument Debug Text` continued with
parser `types/base.cpp` template-arg ref rendering. The local
`render_template_arg_ref` nested-argument rendering and `template_arg_refs_text`
now use structured `TemplateArgRef` payload first: value args render from
`value`, and type args render from the nested `TypeSpec` tag or mangled type.
`debug_text` is only an explicit fallback when the structured type payload
cannot produce a spelling.

Focused coverage now parses `Outer<InnerAlias>` where `InnerAlias` carries a
nested value `TemplateArgRef` with stale `debug_text`. The rendered template
arg ref is `@Inner:7`, proving the parser render path uses the structured
value instead of the stale debug string.

## Suggested Next

Continue Step 3 with the `$expr:` / `template_arg_nttp_names` projection path,
or stop for supervisor review if the remaining NTTP string authority needs a
broader parser/HIR boundary packet.

## Watchouts

`debug_text` is still used as an explicit fallback for structurally unknown
type arguments in parser rendering and the canonical key helper. This packet
intentionally did not touch HIR template materialization or parser-side
`$expr:` / `template_arg_nttp_names` flows. Avoid changing broad `Node::name`
or `TypeSpec::tag` behavior while continuing Step 3.

## Proof

Ran the supervisor-selected proof:
`cmake --build build --target c4cll frontend_parser_tests > test_after.log 2>&1 && ctest --test-dir build -R '^frontend_parser_tests$|template_alias_nttp_expr|template_qualified_nttp_parse|template_type_context_nttp_parse|template_typedef_nttp_variants_parse|qualified_trait_value_template_arg_parse' --output-on-failure >> test_after.log 2>&1`

Result: passed; proof log is `test_after.log`.
