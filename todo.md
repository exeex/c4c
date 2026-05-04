# Current Packet

Status: Complete
Source Idea Path: ideas/open/141_typespec_tag_field_removal_metadata_migration.md
Source Plan Path: plan.md
Current Step ID: 6
Current Step Title: Triage Broad Validation Regression After Field Removal
你該做code review了
你該做test baseline review了

## Just Finished

Step 2 migrated
`test_type_binding_equivalence_uses_deferred_member_text_id_authority` off
direct `TypeSpec::tag` access by carrying the owner identity through
`tag_text_id` plus namespace context while preserving the deferred-member
TextId authority assertions.

## Suggested Next

Migrate the next Step 2 deletion-probe boundary in
`test_template_arg_ref_equivalence_ignores_debug_text_when_structured_payload_matches`,
starting at `tests/frontend/frontend_parser_tests.cpp:7165`, without weakening
the structured template-argument equivalence assertion.

## Watchouts

- Do not reactivate parked idea 142 for parser/HIR fixture residuals.
- Do not weaken tests or remove stale-rendered-spelling disagreement coverage
  just to make the field deletion compile.
- Keep the deferred-member TextId authority checks meaningful: matching member
  TextIds should tolerate stale rendered names, one-sided or mismatched TextIds
  should reject, and rendered compatibility should remain only for the explicit
  no-metadata case.
- `src/frontend/parser/ast.hpp` should only be changed for temporary deletion
  probes unless this becomes the accepted final deletion packet.
- The temporary deletion probe passed the migrated deferred-member fixture and
  first failed at `tests/frontend/frontend_parser_tests.cpp:7165`; follow-on
  direct accesses appeared at `7276`, `7283`, and later consteval/qualified
  TypeSpec metadata fixtures.

## Proof

Ran
`cmake --build build --target frontend_parser_tests > test_after.log 2>&1 && ctest --test-dir build -j --output-on-failure -R '^frontend_parser_tests$' >> test_after.log 2>&1`,
then temporarily removed `TypeSpec::tag` from `src/frontend/parser/ast.hpp` and
ran `cmake --build build --target frontend_parser_tests >> test_after.log 2>&1`
to record the next boundary. Restored `src/frontend/parser/ast.hpp`, then reran
`cmake --build build --target frontend_parser_tests >> test_after.log 2>&1 && ctest --test-dir build -j --output-on-failure -R '^frontend_parser_tests$' >> test_after.log 2>&1`.
The final restored build and `frontend_parser_tests` ctest passed; proof log:
`test_after.log`.
