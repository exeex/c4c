# Current Packet

Status: Complete
Source Idea Path: ideas/open/139_parser_sema_rendered_string_lookup_removal.md
Source Plan Path: plan.md
Current Step ID: Step 2.5
Current Step Title: Preserve Parser Const-Int Boundary And HIR Blocker

## Just Finished

Step 2.5 verified the parser/Sema const-int boundary. Parser-owned named
constant evaluation stays on `TextId`/structured storage through
`Parser::eval_const_int_with_parser_tables()` and direct
`binding_state_.const_int_bindings` calls. The remaining rendered-name
`eval_const_int` compatibility overload is not a parser-owned route to convert
in this packet; its live named-constant use is HIR-backed `NttpBindings`
metadata and remains parked outside this parser/Sema plan.

## Suggested Next

Supervisor should decide whether the parser/Sema rendered-string lookup-removal
runbook is exhausted and route any deletion of the rendered-name
`eval_const_int` compatibility overload through a HIR metadata plan first.

## Watchouts

- Do not delete the rendered-name `eval_const_int` compatibility overload while
  HIR still passes `NttpBindings` as `std::unordered_map<std::string, long long>`.
- Parser `eval_const_int` callers that have named constants already use
  `std::unordered_map<TextId, long long>`; parser three-argument layout calls
  do not perform named-constant lookup.
- No HIR files were edited for this parser/Sema packet.

## Proof

`(cmake --build build -j && ctest --test-dir build -R '^(frontend_parser_tests|cpp_positive_sema_deferred_consteval_nttp_cpp|cpp_positive_sema_template_nttp_default_runtime_cpp|cpp_hir_template_deferred_nttp_expr|cpp_hir_template_deferred_nttp_arith_expr|cpp_hir_template_deferred_nttp_paren_expr|cpp_hir_template_deferred_nttp_bool_expr|cpp_hir_template_deferred_nttp_logic_expr|cpp_hir_template_deferred_nttp_true_expr|cpp_hir_template_deferred_nttp_number_expr|cpp_hir_template_alias_deferred_nttp_static_member|cpp_hir_template_deferred_nttp_static_member_expr|cpp_hir_template_deferred_nttp_cast_static_member_expr|cpp_hir_template_deferred_nttp_sizeof_pack_expr)$' --output-on-failure) > test_after.log 2>&1`

Passed: build succeeded and CTest reported 14/14 matching tests passed.
Canonical proof log: `test_after.log`.
