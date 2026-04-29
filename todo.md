Status: Active
Source Idea Path: ideas/open/134_parser_ast_template_payload_string_bridge_cleanup.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Make Alias-Template Parameter Payloads Structured-Primary

# Current Packet

## Just Finished

Plan Step 2 `Make Alias-Template Parameter Payloads Structured-Primary`
completed for parser alias-template substitution. The alias-template
application path in `types/base.cpp` now derives semantic parameter count from
structured alias metadata (`param_name_text_ids`, NTTP/pack/default vectors)
instead of `param_names.size()`. Substitution still maps parameters by
`TextId`; `param_names` is only consulted when a parameter has no structured
TextId and needs an explicit compatibility spelling fallback.

Focused coverage now includes an alias-template substitution case where
`param_name_text_ids` is present but `param_names` is empty. That proves the
rendered parameter-name payload is not required to bind the aliased `T` type
argument. Existing stale-rendered-name coverage remains in place.

## Suggested Next

Execute Step 3 as a bounded NTTP/template-argument string quarantine packet.
Start with `ParserTemplateArgParseResult::nttp_name`, `$expr:` projection into
`template_arg_nttp_names`, and `TemplateArgRef::debug_text` call sites where a
structured expression node, value, `TypeSpec`, or template parameter TextId is
already available.

## Watchouts

Alias-template metadata can still use `param_names` as an explicit no-TextId
fallback for legacy/test-created `ParserAliasTemplateInfo` records. The next
packet should avoid changing broad `Node::name` or `TypeSpec::tag` behavior
while quarantining NTTP/debug strings; those are still real semantic lookup
inputs in parser/HIR consumers.

## Proof

Ran the supervisor-selected proof:
`cmake --build build --target c4cll frontend_parser_tests > test_after.log 2>&1 && ctest --test-dir build -R '^frontend_parser_tests$|template_alias_nttp_expr|template_alias_qualified_typedef_resolution_parse|namespace_using_decl_template_typedef_type_arg_parse|template_visible_typedef_type_arg_parse' --output-on-failure >> test_after.log 2>&1`

Result: passed; proof log is `test_after.log`.
