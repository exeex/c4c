Status: Active
Source Idea Path: ideas/open/134_parser_ast_template_payload_string_bridge_cleanup.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Quarantine NTTP and Template Argument Debug Text

# Current Packet

## Just Finished

Plan Step 3 `Quarantine NTTP and Template Argument Debug Text` started with a
bounded sema comparison slice. `same_template_arg_ref` in
`sema/type_utils.cpp` now compares template arguments by structured payload:
`TemplateArgKind::Value` uses `value`, and `TemplateArgKind::Type` uses the
structured `TypeSpec`. `TemplateArgRef::debug_text` no longer participates in
this equivalence decision when structured payload exists.

Focused coverage now includes a `type_binding_values_equivalent` case where
two template argument refs have identical structured `kind/value` payloads but
different `debug_text`; they compare equal. This makes the former debug-text
authority path explicit and quarantined for this comparison family.

## Suggested Next

Continue Step 3 with one parser-side keying/projection path. Good candidates
are `canonical_template_struct_type_key` in `types/types_helpers.hpp` or
parser `types/base.cpp` template-arg ref rendering, where `TemplateArgRef`
already carries `kind`, `value`, and `type` but the code still consults
`debug_text` first.

## Watchouts

This packet intentionally did not touch HIR template materialization or
parser-side `$expr:` / `template_arg_nttp_names` flows. Those remain semantic
string authority and should be handled in separate narrow slices. Avoid
changing broad `Node::name` or `TypeSpec::tag` behavior while continuing Step
3.

## Proof

Ran the supervisor-selected proof:
`cmake --build build --target c4cll frontend_parser_tests > test_after.log 2>&1 && ctest --test-dir build -R '^frontend_parser_tests$|template_alias_nttp_expr|template_qualified_nttp_parse|template_type_context_nttp_parse|template_typedef_nttp_variants_parse|qualified_trait_value_template_arg_parse' --output-on-failure >> test_after.log 2>&1`

Result: passed; proof log is `test_after.log`.
