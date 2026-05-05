# Current Packet

Status: Blocked
Source Idea Path: ideas/open/141_typespec_tag_field_removal_metadata_migration.md
Source Plan Path: plan.md
Current Step ID: 6
Current Step Title: Delete TypeSpec Tag And Validate

## Just Finished

Step 6 repaired the rejected dirty HIR route by removing debug-string
`tag_ctx..._textN` rebinding and the new sole-binding type substitution
fallbacks. The remaining HIR repair substitutes nested template-member typedef
arguments through structured `TemplateArgRef.type` / `TypeSpec` carriers and
preserves owner checks before text-key fallback lookup.

The delegated proof is still blocked: four HIR/frontend tests pass, but
`frontend_lir_function_signature_type_ref` still segfaults in the LIR verifier
outside this packet's owned HIR/frontend files.

## Suggested Next

Delegate or authorize a focused LIR verifier/function-signature type-ref carrier
repair starting at `frontend_lir_function_signature_type_ref` and
`verify_function_signature_return_type_ref_mirror`. Keep the current HIR
cleanup as the baseline for that packet; do not restore debug-string rebinding
to make the LIR test move.

## Watchouts

- Do not replace `TypeSpec::tag` with another rendered-string semantic
  authority.
- Preserve structured metadata precedence over stale rendered spelling.
- Do not parse `debug_text` or encoded `tag_ctx..._textN` strings to recover
  template-parameter identity.
- The remaining proof failure is a LIR verifier segfault, not a currently known
  HIR carrier miss inside the owned files.
- Keep the four now-green HIR cases in the delegated subset as regression
  coverage for template member typedefs and deferred consteval type binding.

## Proof

Canonical proof log: `test_after.log`.

Delegated proof command:
`cmake --build build && ctest --test-dir build -j --output-on-failure -R '^(cpp_hir_template_member_owner_signature_local|cpp_hir_template_member_owner_field_and_local|cpp_hir_fixpoint_convergence|cpp_hir_deferred_consteval_chain|frontend_lir_function_signature_type_ref)$' > test_after.log 2>&1`

Result: failed.

`cmake --build build` passed. `ctest` ran the five delegated tests: four passed
and one failed. Passing tests:
`cpp_hir_template_member_owner_signature_local`,
`cpp_hir_template_member_owner_field_and_local`,
`cpp_hir_fixpoint_convergence`, and
`cpp_hir_deferred_consteval_chain`.

Remaining failure: `frontend_lir_function_signature_type_ref` segfaults in
`c4c::codegen::lir::verify_function_signature_return_type_ref_mirror`, which is
outside the owned HIR/frontend files for this packet.

Route-repair check: the owned HIR diff no longer contains the rejected
debug-string rebinding helpers or a new sole-binding semantic fallback for type
substitution.
