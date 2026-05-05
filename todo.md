# Current Packet

Status: Active
Source Idea Path: ideas/open/141_typespec_tag_field_removal_metadata_migration.md
Source Plan Path: plan.md
Current Step ID: 6
Current Step Title: Delete TypeSpec Tag And Validate

## Just Finished

Executed parent Step 6 broad-validation triage after closing the constructor
member-template specialization decomposition.

The full-suite command completed and wrote `test_after.log`. Result: 2999/3023
passing, 24 failing. This red full-suite result is triage data only, not a
canonical accepted baseline.

Representative remaining failure clusters:

- C++ qualified/template parser fallout: EASTL parse recipes and external
  utility fail at `EASTL/internal/type_properties.h:357:46` with `expected
  GREATER but got '<'`; vector/tuple adds `parse_top_level_parameter_list`
  failures at `<`, `::`, and `(`. Related parser-only/debug/perf failures:
  `cpp_parse_template_alias_empty_pack_default_arg_dump`,
  `cpp_parser_debug_qualified_type_template_arg_stack`,
  `cpp_parser_debug_qualified_type_spelling_stack`,
  `cpp_parser_debug_tentative_template_arg_lifecycle`,
  `cpp_parser_debug_tentative_cli_only`, and
  `cpp_qualified_template_call_template_arg_perf` timeout.
- C++ parser/HIR dump mismatch:
  `cpp_parse_record_base_variable_template_value_arg_dump` no longer emits the
  expected `has_unique_object_representations_T_int` specialization dump.
- C aggregate/member-owner frontend failures:
  `c_testsuite_src_00019_c`, `llvm_gcc_c_torture_src_pr40022_c`, and
  `llvm_gcc_c_torture_src_20001124_1_c` fail with `StmtEmitter: field ... not
  found in struct/union ...`.
- LIR structured signature mirror failures:
  `llvm_gcc_c_torture_src_20040709_1_c`,
  `llvm_gcc_c_torture_src_20040709_2_c`,
  `llvm_gcc_c_torture_src_20040709_3_c`, and
  `llvm_gcc_c_torture_src_pr23324_c` report return/parameter mirrors naming a
  different structured type than the aggregate ABI type.
- C runtime segfault cluster:
  `llvm_gcc_c_torture_src_20020402_3_c`,
  `llvm_gcc_c_torture_src_20071018_1_c`,
  `llvm_gcc_c_torture_src_950426_1_c`, and
  `llvm_gcc_c_torture_src_pr41463_c` compile with clang but the c2ll runtime
  exits by segmentation fault.

## Suggested Next

Recommended next Step 6 implementation packet: repair the dominant C++ parser
qualified/template-id fallout first, starting from the shared
`type_properties.h:357` / `expected GREATER but got '<'` boundary and the
related parser debug stack/perf cases. Preserve the structured-authority guard:
do not accept unknown namespace-qualified template ids from spelling alone, and
do not recover semantics from rendered strings or `TypeSpec::tag`.

After that parser packet, rerun a matching broad or focused parent Step 6 proof
and then triage the smaller C aggregate-owner and LIR signature/runtime
clusters separately if they remain.

## Watchouts

- Do not reintroduce `TypeSpec::tag` or rendered-string semantic lookup.
- Treat this red full-suite result as triage data, not canonical accepted
  baseline proof.
- The constructor member-template specialization blocker is closed; do not
  reopen it unless a fresh parent deletion probe exposes a distinct regression
  in the same carrier.
- Split unrelated LIR/BIR/backend/codegen or broad-validation boundaries into
  follow-up open ideas rather than broadening this Step 6 packet.

## Proof

Parent Step 6 broad-validation triage:
`cmake --build build && ctest --test-dir build -j --output-on-failure > test_after.log 2>&1`

Result: failed, 2999/3023 passing with 24 failures.

Proof log: `test_after.log`.

This red result is not an accepted baseline; it is the current parent Step 6
triage snapshot for selecting the next implementation packet.
