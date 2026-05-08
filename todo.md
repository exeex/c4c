Status: Active
Source Idea Path: ideas/open/152_hir_lir_rendered_owner_compatibility_retirement.md
Source Plan Path: plan.md
Current Step ID: 6
Current Step Title: Retire LIR Aggregate Helper Compatibility Tags

# Current Packet

## Just Finished

Completed the delegated Plan Step 6 repair by resolving the template-local aggregate signature/call ABI mismatch that remained after the field-chain compatibility fallback was demoted.

Template-local aggregate value lowering now detects `_tag_ctx`/`_local_text` layouts that have a unique canonical template instantiation with matching semantic layout, and uses the canonical aggregate name for LLVM value/signature/call ABI text. LIR owned signature parameter metadata is canonicalized only for those template-local compatibility carriers, preserving complete owner-key miss rejection and no-owner compatibility behavior.

The earlier field-chain repair remains in place: anonymous-member recursion prefers structured child `StructNameId` metadata and keeps the rendered compatibility route as an explicitly secondary fallback for incomplete metadata.

## Suggested Next

Supervisor should review and decide whether the completed Step 6 slice is ready to commit with the updated proof log, or whether Step 6 needs an independent route review before lifecycle closure.

## Watchouts

- The no-owner compatibility path remains intentionally live for incomplete metadata.
- Owner-key hits normalize aggregate-name mirrors to the structured owner tag; ABI fragments such as `ptr byval(%struct.X)` remain raw mirrors.
- The field-chain anonymous-member compatibility route is now secondary and documented, but union anonymous-member recursion still cannot recover child layout ids from the current union LIR layout shape.
- Template-local canonicalization is intentionally constrained to `_tag_ctx`/`_local_text` compatibility layouts with a unique canonical template instantiation candidate. The stale complete-owner-miss test in `frontend_lir_function_signature_type_ref` passed and should stay part of the acceptance proof.

## Proof

The delegated proof command passed, and the accepted `test_after.log` was rolled forward to `test_before.log` after supervisor regression-guard review. The command was:

`cmake --build --preset default --target c4cll frontend_lir_call_type_ref_test frontend_lir_function_signature_type_ref_test frontend_lir_global_type_ref_test frontend_lir_extern_decl_type_ref_test && ctest --test-dir build -R '^(frontend_lir_call_type_ref|frontend_lir_function_signature_type_ref|frontend_lir_global_type_ref|frontend_lir_extern_decl_type_ref|cpp_positive_sema_template_(angle_bracket_validation|struct_advanced|struct_nested)_cpp)$' --output-on-failure > test_after.log`

Accepted proof result:

- passing: `frontend_lir_call_type_ref`, `frontend_lir_function_signature_type_ref`, `frontend_lir_global_type_ref`, `frontend_lir_extern_decl_type_ref`, `cpp_positive_sema_template_angle_bracket_validation_cpp`, `cpp_positive_sema_template_struct_advanced_cpp`, `cpp_positive_sema_template_struct_nested_cpp`
- failing: none
