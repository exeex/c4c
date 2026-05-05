# Current Packet

Status: Active
Source Idea Path: ideas/open/141_typespec_tag_field_removal_metadata_migration.md
Source Plan Path: plan.md
Current Step ID: 6
Current Step Title: Delete TypeSpec Tag And Validate

## Just Finished

Step 6's focused implementation packet repaired the remaining LIR
function-signature type-ref carrier failure after `TypeSpec::tag` deletion.
LIR lowering now snapshots aggregate signature `TypeSpec` metadata into the
LIR module text table and drops frontend-owned `record_def` / qualifier
pointer carriers before storing the type on `LirFunction` metadata. The LIR
verifier no longer dereferences those frontend-owned pointers when checking
signature mirrors; it derives expected aggregate ids from the LIR-owned
`tag_text_id` carrier and keeps rejecting direct known aggregate mirrors
without `StructNameId`. Byval aggregate signature parameters with no direct
`StructNameId` must still carry an aggregate ABI fragment instead of silently
accepting scalar drift.

Lifecycle close is not accepted yet. The post-deletion broad validation
required by Step 6's completion check has not run after the focused repair.

## Suggested Next

Run a supervisor-selected broad post-deletion validation packet before
lifecycle close. At minimum, rebuild and run the broad frontend/HIR validation
needed to cover `TypeSpec::tag` removal after the final Step 6 repair; if this
is treated as the close gate for the source idea, run the full suite or an
equivalent close-scope regression guard. Only close the idea after that broad
proof is green, or split any remaining failures into follow-up open ideas.

Do not treat the focused five-test proof as close-sufficient by itself.

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

Canonical focused proof log: `test_after.log`.

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

Broad post-deletion frontend/HIR or full-suite validation after the final Step
6 repair: not run yet; this is the next required lifecycle blocker before
closing `ideas/open/141_typespec_tag_field_removal_metadata_migration.md`.
