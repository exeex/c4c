Status: Active
Source Idea Path: ideas/open/145_move_record_tag_authority_from_parser_to_sema.md
Source Plan Path: plan.md
Current Step ID: 4C
Current Step Title: Migrate Remaining Parser Record Lookup Families

# Current Packet

## Just Finished

Step 4C regression repair completed. Declaration-time incomplete
object checks now treat only complete direct `record_def` as authoritative, then
repair C typedef-backed records by following the typed typedef binding to a
complete record whose `unqualified_text_id` matches the typedef target, without
using rendered parser-map keys as semantic authority. This clears the five
LLVM GCC torture C regressions for `_RenderInfo`, `__db_lsn`,
`__db_txnlist`, `PgHdr`, and `cpp_num`.

HIR template primary lookup now rejects rendered origin/tag fallback after a
complete structured origin-key miss. This restores
`cpp_hir_template_canonical_primary_origin_structured_metadata` while
preserving the no-rendered-recovery rule.

Parser nested `TemplateArgRef` traversals now have bounded recursion and
template-argument-array cycle guards in the origin-spelling normalization,
argument construction, and template-parameter annotation paths. This fixes the
four remaining parse-only C++ segfaults while keeping structured typed carriers
intact and without adding `debug_text`, rendered template instance, or rendered
record-name identity recovery.

## Suggested Next

Supervisor should review and commit this Step 4C regression repair slice with
`src/frontend/parser/impl/declarations.cpp`,
`src/frontend/parser/impl/types/base.cpp`,
`src/frontend/hir/impl/templates/templates.cpp`, and this `todo.md` update.
The untracked `review/step4c_repair_route_review.md` remains outside this
packet.

## Watchouts

The C typedef repair iterates parser record storage only as a typed
`TextId`-matched carrier lookup and only in C mode; it does not authorize C++
structured tag/context/qualifier carriers through rendered map-key recovery.
Existing focused parser tests for stale structured tag-only declaration
rejection still pass.

The traversal guards stop cyclic carrier walks by recursion depth and by
re-entered `TemplateArgRefList::data` arrays. They do not infer record/template
identity from rendered strings; origin spelling is still populated only from
typed template primary lookup by structured key or existing primary metadata.

## Proof

Proof run:

`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(frontend_parser_tests|frontend_parser_lookup_authority_tests|cpp_hir_parser_declarations_residual_structured_metadata|cpp_hir_parser_type_base_residual_structured_metadata|cpp_eastl_vector_parse_recipe|negative_tests_bad_sizeof_incomplete_struct_type|cpp_positive_sema_template_struct_advanced_cpp|cpp_hir_template_canonical_primary_origin_structured_metadata|cpp_positive_sema_iterator_concepts_following_hash_base_parse_cpp|cpp_positive_sema_stl_iterator_then_max_size_type_parse_cpp|cpp_std_vector_parse_debug_progress|cpp_std_vector_parse_debug_progress_file|llvm_gcc_c_torture_src_20000605_1_c|llvm_gcc_c_torture_src_20011008_3_c|llvm_gcc_c_torture_src_20040703_1_c|llvm_gcc_c_torture_src_pr33870_1_c|llvm_gcc_c_torture_src_pr33870_c)$' > test_after.log`

Result: passed, 17/17 tests. Proof log: `test_after.log`.
