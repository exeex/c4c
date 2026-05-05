# Current Packet

Status: Active
Source Idea Path: ideas/open/143_typespec_identity_normalization_boundary.md
Source Plan Path: plan.md
Current Step ID: 6
Current Step Title: Triage Broad Validation Regression After Field Removal

## Just Finished

Step 6 broad-validation triage fixed the current parser unit regression from
the post-Step-4 field-removal route. Visible lexical aliases used as template
type arguments still classify as type arguments and resolve through their
scope-local target type, but `parse_base_type()` no longer restores the
using-alias target `TextId` as fabricated typedef metadata after scalar typedef
expansion.

`review/143_step6_visible_alias_metadata_review.md` found no blocking issues:
the visible using-alias metadata repair remains source-aware, preserves the
Step 4 dependent `typename`/cast identity route, and can continue under Step 6.

## Suggested Next

Continue Step 6 resume triage against the remaining broad-validation failures.
The close bar for idea 143 is still full green:
`ctest --test-dir build -j 8 --output-on-failure` must pass with zero failures,
not merely show fewer failures than the current baseline.

## Watchouts

- Do not delete `TypeSpec::tag` during this runbook.
- Do not add another local rendered-tag fallback in a consumer as progress.
- Preserve stale-rendered-spelling disagreement tests.
- Rejected baseline candidate: accepting the visible lexical alias
  type-argument failure as baseline drift is not valid because
  `frontend_parser_tests` contains the intended non-fabrication guard and the
  regression is local to parser TypeSpec metadata production.
- Keep idea 141 parked until normalized identity boundaries are stable.
- Do not close idea 143 while full ctest still has failures.

## Proof

Accepted proof is rolled forward in `test_before.log`:
`cmake --build build --target frontend_parser_tests cpp_hir_parser_declarator_deferred_owner_metadata_test c4cll && ctest --test-dir build -j --output-on-failure -R '^(frontend_parser_tests|cpp_hir_parser_declarator_deferred_owner_structured_metadata|cpp_positive_sema____generated_parser_disambiguation_matrix_compile_positive_owner_dependent_(template_member|typename)__decl_(function_lvalue_ref|function_pointer|function_rvalue_ref|member_function_pointer)__ctx_c_style_cast_target__compile_positive_cpp)$'`.

The subset ran 10 tests and all passed, including `frontend_parser_tests`, the
Step 4 deferred-owner focused test, and the Group D c-style-cast matrix cases.
