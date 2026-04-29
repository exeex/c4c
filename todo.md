Status: Active
Source Idea Path: ideas/open/134_parser_ast_template_payload_string_bridge_cleanup.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Quarantine NTTP and Template Argument Debug Text

# Current Packet

## Just Finished

Plan Step 3 `Quarantine NTTP and Template Argument Debug Text` continued by
quarantining HIR `TemplateArgRef::debug_text` authority in template
materialization, pending-type keys, and template type rendering. HIR template
argument encoders now prefer structured value/type payloads and only use
`debug_text` as display or structurally unknown fallback text.

Focused HIR coverage now materializes `Box<7>` with stale rendered text
`RenderedN` bound to `101`; `materialize_template_args` records `N = 7`,
proving stale `TemplateArgRef::debug_text` cannot override a structured value
payload.

## Suggested Next

Continue Step 3 by reviewing parser-to-HIR producers that create zero-valued
legacy forwarded NTTP refs, then decide whether a structured presence bit is
needed to distinguish real `0` payloads from debug-text-only fallback refs.

## Watchouts

Plain forwarded NTTP identifiers still use a zero value plus `debug_text` as
the compatibility carrier, so HIR materialization and type-resolution keep that
zero-valued fallback path. Nonzero structured values and concrete structured
types now bypass stale rendered text. `debug_text` remains an explicit fallback
for deferred `$expr:` refs and structurally unknown type args.

## Proof

Ran the supervisor-selected proof:
`cmake --build build --target c4cll frontend_hir_tests frontend_parser_tests > test_after.log 2>&1 && ctest --test-dir build -R '^frontend_hir_tests$|^frontend_parser_tests$|cpp_hir_template_struct_arg_materialization|cpp_hir_template_deferred_nttp_expr|cpp_hir_template_deferred_nttp_arith_expr|cpp_hir_template_deferred_nttp_bool_expr|cpp_hir_template_deferred_nttp_static_member_expr|template_alias_nttp_expr|template_qualified_nttp_parse|qualified_trait_value_template_arg_parse|variadic_template_arg_sizeof_pack_parse' --output-on-failure >> test_after.log 2>&1`

Result: passed; proof log is `test_after.log`.
