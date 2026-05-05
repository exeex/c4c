# Current Packet

Status: Active
Source Idea Path: ideas/open/141_typespec_tag_field_removal_metadata_migration.md
Source Plan Path: plan.md
Current Step ID: 6
Current Step Title: Delete TypeSpec Tag And Validate

## Just Finished

Repaired the over-broad Step 6 array-object lvalue indexing follow-up from
commit `c8c53a368`.

StmtEmitter `IndexExpr` lvalue lowering now preserves lvalue bases only for
storage-backed array objects: member arrays, nested array-index lvalues,
dereferenced pointer-to-array expressions, globals, and locals whose actual
slot type is an array object. Adjusted parameters, VLAs, pointer slots, and
other pointer-valued expressions remain on the rvalue/decay path.

The module-aware LIR alloca helper now emits outer array storage before
collapsing pointer/function-pointer element types to `ptr`, so arrays of
pointers and function pointers allocate `[N x ptr]` storage instead of a single
pointer slot. Lvalue dereference now clears `is_ptr_to_array` after consuming
the final pointer layer, matching the existing expression type resolver and
restoring typed indexing for `(*p)[i]`.

The exact delegated 13-test regression subset improved from 0/13 to 13/13.
The original four runtime segfault fixes from `c8c53a368` and the prior
metadata spot checks remain green.

## Suggested Next

Recommended next Step 6 packet: supervisor-side broad/full validation for the
parent TypeSpec-tag deletion route. The owned array/indexing/VLA/string
regression subset and the original runtime segfault cluster are green.

## Watchouts

- Do not reintroduce `TypeSpec::tag` or rendered-string semantic lookup.
- C array fields whose element type is a pointer have `ptr_level > 0` and
  `outer_array_rank > 0`; they are still array objects and must not be loaded
  before indexing.
- C function parameters declared with array syntax are adjusted pointer
  parameters. They may still carry array-rank metadata, so decl-ref parameter
  bases must not be forced through the array-object lvalue indexing path.
- Arrays of pointer or function-pointer elements still need real array storage
  when they are local/global/member array objects; do not collapse outer array
  dimensions to `ptr` before alloca/layout emission.
- Pointer-to-array dereference consumes the pointer wrapper. The resulting
  lvalue is an array object, so `is_ptr_to_array` must be cleared when the
  final pointer layer is removed.
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

Step 6 delegated array/indexing regression proof:
`cmake --build build && ctest --test-dir build --output-on-failure -R '^(positive_sema_inline_phase9_c|positive_sema_ok_expr_canonical_cast_index_c|llvm_gcc_c_torture_src_20180921_1_c|llvm_gcc_c_torture_src_strlen_4_c|llvm_gcc_c_torture_src_20040811_1_c|llvm_gcc_c_torture_src_20080424_1_c|llvm_gcc_c_torture_src_20090814_1_c|llvm_gcc_c_torture_src_920929_1_c|llvm_gcc_c_torture_src_pr43220_c|llvm_gcc_c_torture_src_pr58277_1_c|llvm_gcc_c_torture_src_pr58277_2_c|llvm_gcc_c_torture_src_pr58831_c|llvm_gcc_c_torture_src_vla_dealloc_1_c)$' > test_after.log 2>&1`

Result: passed, 13/13. The rejected full-suite follow-up regressions for
array parameters, pointer arrays, pointer-to-array indexing, strings, and VLAs
are fixed.

Proof log: `test_after.log`.

Original four `c8c53a368` runtime checks:
`ctest --test-dir build --output-on-failure -R '^(llvm_gcc_c_torture_src_20020402_3_c|llvm_gcc_c_torture_src_20071018_1_c|llvm_gcc_c_torture_src_950426_1_c|llvm_gcc_c_torture_src_pr41463_c)$'`

Result: passed, 4/4.

Prior metadata spot checks:
`ctest --test-dir build --output-on-failure -R '^(frontend_parser_tests|cpp_hir_parser_type_base_prelim_eval_structured_metadata|c_testsuite_src_00019_c|llvm_gcc_c_torture_src_20040709_1_c|llvm_gcc_c_torture_src_pr40022_c|llvm_gcc_c_torture_src_pr23324_c)$'`

Result: passed, 6/6.
