# Current Packet

Status: Active
Source Idea Path: ideas/open/139_parser_sema_rendered_string_lookup_removal.md
Source Plan Path: plan.md
Review Artifact Path: none
Current Step ID: Step 3.1
Current Step Title: Add Consteval Local And TypeSpec Metadata Producers

## Just Finished

Step 3.1 audited the consteval function-body value producer route for
parameters, local declarations, assignment targets, and `for` initializers.
The covered route already populates metadata before rendered-name compatibility
can matter: `parse_param()` gives parameter declarations `unqualified_text_id`,
`parse_local_decl()` gives local and `for` declaration nodes
`unqualified_text_id`, expression parsing applies qualified-name metadata to
identifier references through `Parser::apply_qualified_name()`, and
`InterpreterBindings::bind()` records the resulting value in `by_text` and
`by_key` alongside the rendered-name map.

No focused producer repair was needed for this function-body value slice.

## Suggested Next

Route the covered consteval function-body value lookup path to Step 3.2 for
fallback deletion, while leaving non-body/global consteval value routes and the
already-covered TypeSpec route to the supervisor's Step 3.2 packet selection.

## Watchouts

- This packet did not delete `ConstEvalEnv` rendered compatibility; Step 3.2
  still owns removing the rendered fallback after producer completeness is
  accepted.
- The audit was scoped to function-body interpreter bindings:
  `evaluate_consteval_call()` parameter binding, `NK_DECL` locals including
  `for` initializers, and `NK_ASSIGN` targets. It did not claim completeness
  for unrelated global/named const or enum consteval value maps.
- HIR-owned metadata carriers and `src/frontend/hir/*` remain untouched and out
  of scope.

## Proof

`cmake --build --preset default > test_after.log 2>&1 && ctest --test-dir build -j --output-on-failure -R '(^cpp_positive_sema_.*consteval.*_cpp$|^cpp_negative_tests_.*consteval)' >> test_after.log 2>&1`

Result: passed. `test_after.log` contains a fresh successful build plus 28/28
passing delegated consteval positive/negative tests, including
`cpp_positive_sema_consteval_typespec_member_alias_cpp`.
