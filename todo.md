Status: Active
Source Idea Path: ideas/open/134_parser_ast_template_payload_string_bridge_cleanup.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Quarantine NTTP and Template Argument Debug Text

# Current Packet

## Just Finished

Plan Step 3 `Quarantine NTTP and Template Argument Debug Text` continued with
the regression repair for the AST expression payload side of parser `$expr:` /
`template_arg_nttp_names`. Consteval call binding, HIR consteval-call lowering,
HIR call NTTP deduction, and AST template value-arg resolution now evaluate
`template_arg_exprs` first and only bypass the rendered-name path when that
structured evaluation succeeds.

Focused coverage now builds a callee whose structured template arg expression
evaluates to `Structured + 1 == 7` while the stale rendered mirror names
`$expr:Rendered+1 == 101`. The consteval binding records `N = 7`, proving the
rendered `$expr:` string cannot select the semantic payload when the structured
expression succeeds. Failed structured evaluation now falls through to the
existing deferred fallback path, which repairs the variadic `sizeof...` pack
case.

## Suggested Next

Continue Step 3 by reviewing remaining string fallbacks that lack a structured
AST expression payload, especially HIR template materialization paths that still
decode `TemplateArgRef::debug_text`.

## Watchouts

Plain forwarded NTTP identifiers still use `template_arg_nttp_names` because no
structured expression node is produced for that legacy path. This packet only
quarantines rendered-name authority when `template_arg_exprs[index]` evaluates
successfully. `debug_text` remains an explicit fallback for structurally unknown
type args.

## Proof

Ran the supervisor-selected proof:
`cmake --build build --target c4cll frontend_parser_tests > test_after.log 2>&1 && ctest --test-dir build -R '^frontend_parser_tests$|template_alias_nttp_expr|template_qualified_nttp_parse|template_type_context_nttp_parse|template_typedef_nttp_variants_parse|qualified_trait_value_template_arg_parse|variadic_template_arg_sizeof_pack_parse' --output-on-failure >> test_after.log 2>&1`

Result: passed; proof log is `test_after.log`.
