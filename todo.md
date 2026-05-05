# Current Packet

Status: Complete
Source Idea Path: ideas/open/141_typespec_tag_field_removal_metadata_migration.md
Source Plan Path: plan.md
Current Step ID: 6
Current Step Title: Delete TypeSpec Tag And Validate

## Just Finished

Step 6 repaired the remaining LIR function-signature type-ref carrier failure
after `TypeSpec::tag` deletion. LIR lowering now snapshots aggregate
signature `TypeSpec` metadata into the LIR module text table and drops
frontend-owned `record_def` / qualifier pointer carriers before storing the
type on `LirFunction` metadata. The LIR verifier no longer dereferences those
frontend-owned pointers when checking signature mirrors; it derives expected
aggregate ids from the LIR-owned `tag_text_id` carrier and keeps rejecting
direct known aggregate mirrors without `StructNameId`. Byval aggregate
signature parameters with no direct `StructNameId` must still carry an
aggregate ABI fragment instead of silently accepting scalar drift.

## Suggested Next

Supervisor can review and commit this coherent Step 6 slice with
`src/codegen/lir/verify.cpp`, `src/codegen/lir/hir_to_lir/hir_to_lir.cpp`,
and `todo.md`; `test_after.log` is preserved on disk as the canonical proof
log. Supervisor can then decide whether Step 6 needs broader validation before
lifecycle close.

## Watchouts

- Do not replace `TypeSpec::tag` with another rendered-string semantic
  authority.
- Preserve structured metadata precedence over stale rendered spelling.
- Do not parse `debug_text` or encoded `tag_ctx..._textN` strings to recover
  template-parameter identity.
- The LIR verifier repair intentionally uses LIR-owned `tag_text_id` and
  `StructNameId` carriers, not parser-owned `record_def` pointers.
- Keep the four HIR cases in the delegated subset as regression coverage for
  template member typedefs and deferred consteval type binding.

## Proof

Canonical proof log: `test_after.log`.

Delegated proof command:
`cmake --build build && ctest --test-dir build -j --output-on-failure -R '^(cpp_hir_template_member_owner_signature_local|cpp_hir_template_member_owner_field_and_local|cpp_hir_fixpoint_convergence|cpp_hir_deferred_consteval_chain|frontend_lir_function_signature_type_ref)$' > test_after.log 2>&1`

Result: passed.

`cmake --build build` passed. `ctest` ran the five delegated tests and all
passed:
`cpp_hir_template_member_owner_signature_local`,
`cpp_hir_template_member_owner_field_and_local`,
`cpp_hir_fixpoint_convergence`, and
`cpp_hir_deferred_consteval_chain`, plus
`frontend_lir_function_signature_type_ref`.
