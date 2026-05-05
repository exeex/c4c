# Current Packet

Status: Active
Source Idea Path: ideas/open/141_typespec_tag_field_removal_metadata_migration.md
Source Plan Path: plan.md
Current Step ID: 6
Current Step Title: Delete TypeSpec Tag And Validate

## Just Finished

Repaired the Step 6 C/torture structured function signature mirror mismatch
cluster in HIR-to-LIR lowering.

`lir_owned_type_spec` now receives the HIR module and derives aggregate
return/parameter ownership from `find_typespec_aggregate_layout` before
falling back to compatibility spelling. This keeps the LIR function
`return_type` and `signature_params` mirrors aligned with the same structured
record identity used by `signature_*_type_refs`, instead of carrying a stale
`record_def`-derived tag after `TypeSpec::tag` removal.

The delegated C/torture proof improved from 0/4 to 4/4 passing, and the
requested parser/HIR metadata spot checks remain green.

## Suggested Next

Recommended next Step 6 packet: supervisor-side broad/full validation for the
parent TypeSpec-tag deletion route. This LIR signature mirror slice is green
for the owned C/torture cluster; remaining full-suite failures should be
triaged as separate Step 6 packets.

## Watchouts

- Do not reintroduce `TypeSpec::tag` or rendered-string semantic lookup.
- LIR function signature mirrors should derive aggregate identity from HIR
  structured layouts first. Do not restore direct `tag_text_id`/stale
  `record_def` spelling as the semantic source for owned function
  return/parameter carriers.
- The namespace-qualified unknown-template guard was checked separately with
  `frontend_parser_tests`, which passed.
- The prior qualified alias-template fix remains green in this proof; preserve
  the structured `ParserAliasTemplateInfo` authority and the unknown
  namespace-qualified template-id guard.
- The `Owner<Args>::member` repair only succeeds after structured template
  owner lookup plus selected-member typedef resolution; unknown member names
  still roll back.
- The prior perf guard's repeated full tentative-expression parsing in
  qualified template-call default arguments is fixed by the structured carrier
  in this packet; preserve the focused carrier shape rather than returning to
  raw token capture.
- This packet did not add rollback/lite-guard changes.
- Variable-template value-argument evaluation now depends on structured
  template-global registration plus owner-aware template-parameter metadata;
  preserve owner/index checks so nested templates with the same parameter name
  do not bind to the wrong scope.
- Explicit NTTP argument preliminary evaluation must not see arbitrary current
  template bindings. The local same-primary rebuild is intentionally limited to
  direct functional-cast carriers and skips text ids already visible in the
  saved outer bindings, so unrelated trait expressions such as struct-base
  owner checks remain deferred instead of becoming parser-owned placeholders.
- Concrete owner materialization is limited to structured, concrete template
  owner arguments. Dependent owner arguments, including `sizeof...(pack)`,
  still remain deferred for later specialization.
- The constructor member-template specialization blocker is closed; do not
  reopen it unless a fresh parent deletion probe exposes a distinct regression
  in the same carrier.
- Split unrelated LIR/BIR/backend/codegen or broad-validation boundaries into
  follow-up open ideas rather than broadening this Step 6 packet.

## Proof

Step 6 delegated C/torture signature mirror proof:
`cmake --build build && ctest --test-dir build --output-on-failure -R '^(llvm_gcc_c_torture_src_20040709_1_c|llvm_gcc_c_torture_src_20040709_2_c|llvm_gcc_c_torture_src_20040709_3_c|llvm_gcc_c_torture_src_pr23324_c)$' > test_after.log 2>&1`

Result: passed, 4/4. The return mirror mismatches for `retmeC`/`retmeG` and
the parameter mirror mismatch for `callee_af7` are fixed.

Proof log: `test_after.log`.

Required spot checks:
`ctest --test-dir build --output-on-failure -R '^(frontend_parser_tests|cpp_hir_parser_type_base_prelim_eval_structured_metadata)$'`

Result: passed, 2/2.
