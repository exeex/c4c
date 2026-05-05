# Current Packet

Status: Active
Source Idea Path: ideas/open/141_typespec_tag_field_removal_metadata_migration.md
Source Plan Path: plan.md
Current Step ID: 6
Current Step Title: Delete TypeSpec Tag And Validate

## Just Finished

Repaired the Step 6 C/torture runtime segfault cluster caused by array-object
indexing through C aggregate fields.

StmtEmitter `IndexExpr` lvalue lowering now keeps true array-object bases on the
lvalue path before emitting the indexed GEP. This preserves the address of
local/global/member array objects such as `field[i]` instead of loading the
first array element and then treating that element value as the base pointer.
Adjusted function-parameter array declarators remain on the existing rvalue
pointer path, so `param[i]` still indexes the adjusted pointer value.

The repair is structural TypeSpec-driven lowering via `outer_array_rank`, with
an explicit adjusted-parameter guard. It does not use rendered names, testcase
names, or legacy `TypeSpec::tag` recovery.

## Suggested Next

Recommended next Step 6 packet: supervisor-side broad/full validation for the
parent TypeSpec-tag deletion route. The owned C/torture runtime segfault cluster
is green; any remaining load-sensitive parser perf failure should stay split
from backend runtime lowering.

## Watchouts

- Do not reintroduce `TypeSpec::tag` or rendered-string semantic lookup.
- C array fields whose element type is a pointer have `ptr_level > 0` and
  `outer_array_rank > 0`; they are still array objects and must not be loaded
  before indexing.
- C function parameters declared with array syntax are adjusted pointer
  parameters. They may still carry array-rank metadata, so decl-ref parameter
  bases must not be forced through the array-object lvalue indexing path.
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

Step 6 delegated runtime segfault proof:
`cmake --build build && ctest --test-dir build --output-on-failure -R '^(llvm_gcc_c_torture_src_20020402_3_c|llvm_gcc_c_torture_src_20071018_1_c|llvm_gcc_c_torture_src_950426_1_c|llvm_gcc_c_torture_src_pr41463_c)$' > test_after.log 2>&1`

Result: passed, 4/4. The runtime segfaults in the delegated C/torture cluster
are fixed.

Proof log: `test_after.log`.

Required spot checks:
`ctest --test-dir build --output-on-failure -R '^(frontend_parser_tests|cpp_hir_parser_type_base_prelim_eval_structured_metadata|c_testsuite_src_00019_c|llvm_gcc_c_torture_src_20040709_1_c|llvm_gcc_c_torture_src_pr40022_c|llvm_gcc_c_torture_src_pr23324_c)$'`

Result: passed, 6/6.
