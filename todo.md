# Current Packet

Status: Active
Source Idea Path: ideas/open/143_typespec_identity_normalization_boundary.md
Source Plan Path: plan.md
Current Step ID: 6
Current Step Title: Triage Broad Validation Regression After Field Removal

## Just Finished

Step 6 broad-validation triage fixed the delegated parser/EASTL EOF bucket
where dependent `typename conditional<...>::type` using-aliases corrupted
nested `>>` template-close tokens during the `parse_type_name()` leading
`typename` probe. The probe now restores parser token mutations through the
existing tentative guard, and the EASTL-shaped alias has a focused frontend
regression test.

## Suggested Next

Continue Step 6 resume triage against the remaining broad-validation failures
outside this EOF bucket. The close bar for idea 143 is still full green:
`ctest --test-dir build -j 8 --output-on-failure` must pass with zero failures,
not merely this now-green delegated subset.

## Watchouts

- Do not delete `TypeSpec::tag` during this runbook.
- Do not add another local rendered-tag fallback in a consumer as progress.
- Preserve stale-rendered-spelling disagreement tests.
- The repaired EOF family came from manual cursor-only rollback after a
  dependent-typename probe; future parser probes that can call
  `match_template_close()` need real tentative-state restoration so split
  `>>`, `>=`, or `>>=` mutations do not leak.
- Rejected baseline candidate: accepting the visible lexical alias
  type-argument failure as baseline drift is not valid because
  `frontend_parser_tests` contains the intended non-fabrication guard and the
  regression is local to parser TypeSpec metadata production.
- Keep idea 141 parked until normalized identity boundaries are stable.
- Do not close idea 143 while full ctest still has failures.

## Proof

Accepted proof is in `test_before.log`:
`cmake --build build --target frontend_parser_tests cpp_hir_parser_declarator_deferred_owner_metadata_test c4cll && ctest --test-dir build -j --output-on-failure -R '^(frontend_parser_tests|cpp_hir_parser_declarator_deferred_owner_structured_metadata|cpp_positive_sema_(eastl_slice7_piecewise_ctor_parse|iterator_concepts_following_hash_base_parse|stl_iterator_then_max_size_type_parse)_cpp|cpp_eastl_tuple_fwd_decls_parse_recipe|cpp_positive_sema____generated_parser_disambiguation_matrix_compile_positive_owner_dependent_(template_member|typename)__decl_(function_lvalue_ref|function_pointer|function_rvalue_ref|member_function_pointer)__ctx_c_style_cast_target__compile_positive_cpp)$'`.

The delegated subset ran 14 tests and all passed, including the four prior EOF
failures, `frontend_parser_tests`, the Step 4 deferred-owner focused test, and
the dependent `typename`/cast matrix guards.
