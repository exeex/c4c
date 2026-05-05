# Current Packet

Status: Active
Source Idea Path: ideas/open/141_typespec_tag_field_removal_metadata_migration.md
Source Plan Path: plan.md
Current Step ID: 6
Current Step Title: Delete TypeSpec Tag And Validate

## Just Finished

Repaired the Step 6 C/torture StmtEmitter structured field lookup cluster for
C aggregate fields whose parser TypeSpecs carried only parser-origin tag text
ids.

HIR struct field lowering now canonicalizes aggregate field TypeSpecs by
matching parser `unqualified_text_id` metadata against already registered
struct owner keys when `record_def` is absent. The canonical owner key updates
the field TypeSpec's HIR `tag_text_id`, namespace metadata, and `record_def`
carrier before the field is stored in `HirStructField`.

This repairs recursive self-pointer fields, anonymous/forward struct pointer
fields, and forward-declared C tag fields without falling back to HIR text-table
spelling. The delegated proof improved from 0/3 to 3/3 passing, and the
requested parser/HIR plus prior LIR signature spot checks remain green.

## Suggested Next

Recommended next Step 6 packet: supervisor-side broad/full validation for the
parent TypeSpec-tag deletion route. This StmtEmitter field lookup slice is
green for the owned C/torture/C-testsuite cluster; remaining full-suite
failures should be triaged as separate Step 6 packets.

## Watchouts

- Do not reintroduce `TypeSpec::tag` or rendered-string semantic lookup.
- LIR function signature mirrors should derive aggregate identity from HIR
  structured layouts first. Do not restore direct `tag_text_id`/stale
  `record_def` spelling as the semantic source for owned function
  return/parameter carriers.
- HIR struct field TypeSpecs may arrive from parser with parser-table
  `tag_text_id` and no `record_def`, especially for self/forward C tags. Keep
  the structured owner-key canonicalization before field metadata is stored, so
  StmtEmitter does not recover aggregate identity from rendered text.
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

Step 6 delegated StmtEmitter field lookup proof:
`cmake --build build && ctest --test-dir build --output-on-failure -R '^(c_testsuite_src_00019_c|llvm_gcc_c_torture_src_pr40022_c|llvm_gcc_c_torture_src_20001124_1_c)$' > test_after.log 2>&1`

Result: passed, 3/3. The field lookup failures for `p`, `a`, and
`s_blocksize` are fixed.

Proof log: `test_after.log`.

Required spot checks:
`ctest --test-dir build --output-on-failure -R '^(frontend_parser_tests|cpp_hir_parser_type_base_prelim_eval_structured_metadata|llvm_gcc_c_torture_src_20040709_1_c|llvm_gcc_c_torture_src_pr23324_c)$'`

Result: passed, 4/4.
