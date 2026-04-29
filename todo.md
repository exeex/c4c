Status: Active
Source Idea Path: ideas/open/134_parser_ast_template_payload_string_bridge_cleanup.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Quarantine NTTP and Template Argument Debug Text

# Current Packet

## Just Finished

Plan Step 3 `Quarantine NTTP and Template Argument Debug Text` completed the
parser-side regression repair for base-instantiation template argument
carriers. The base-instantiation path now refuses to consume zero-valued
nonnumeric `TemplateArgRef` value carriers as literal `0`, and it also routes
unstructured type-kind debug carriers such as `T` through the existing
debug-text fallback instead of rendering them as default `void`.

Focused HIR coverage now includes pending-type key encoding for zero/false
legacy refs plus nonzero stale debug text, and materialization coverage for a
zero-valued forwarded NTTP binding and a false forwarded NTTP binding. The
delegated parse/HIR/runtime subset now passes.

## Suggested Next

Supervisor review and commit the completed Step 3 slice, including the prior
HIR compatibility-carrier repairs, the parser base-instantiation fallback, and
the focused HIR tests.

## Watchouts

The fallback remains intentionally narrow: structured nonzero value refs stay
value-primary, concrete structured type refs stay type-primary, and `$expr:`
refs still use the existing deferred-expression path. Zero-valued debug-text
carriers remain a compatibility fallback until the AST/HIR boundary has a real
presence bit for value payloads.

## Proof

Ran the supervisor-selected proof:
`cmake --build build --target c4cll frontend_hir_tests frontend_parser_tests > test_after.log 2>&1 && ctest --test-dir build -R '^frontend_hir_tests$|^frontend_parser_tests$|cpp_positive_sema_inherited_operator_call_temporary_runtime_cpp|cpp_positive_sema_template_alias_deferred_nttp_expr_runtime_cpp|cpp_positive_sema_template_struct_advanced_cpp|cpp_positive_sema_template_struct_nested_cpp|cpp_positive_sema_template_variable_alias_member_typedef_runtime_cpp|cpp_hir_template_struct_body_instantiation|cpp_hir_template_alias_deferred_nttp_static_member|cpp_hir_template_inherited_member_typedef_trait|cpp_hir_template_struct_arg_materialization|cpp_hir_template_deferred_nttp_expr|cpp_hir_template_deferred_nttp_arith_expr|cpp_hir_template_deferred_nttp_bool_expr|cpp_hir_template_deferred_nttp_static_member_expr|template_alias_nttp_expr|template_qualified_nttp_parse|template_type_context_nttp_parse|template_typedef_nttp_variants_parse|qualified_trait_value_template_arg_parse|variadic_template_arg_sizeof_pack_parse' --output-on-failure >> test_after.log 2>&1`

Result: passed, 22/22 delegated tests green; proof log is `test_after.log`.
