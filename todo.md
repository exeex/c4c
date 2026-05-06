Status: Active
Source Idea Path: ideas/open/145_move_record_tag_authority_from_parser_to_sema.md
Source Plan Path: plan.md
Current Step ID: 4C
Current Step Title: Migrate Remaining Parser Record Lookup Families

# Current Packet

## Just Finished

Step 4C template static-member/base lookup migration completed for
`Parser::eval_deferred_nttp_expr_tokens`. Recursive base traversal now resolves
structured base record carriers through direct complete `record_def` or
`definition_state_.struct_defs` matches that require record kind, namespace
context, global qualification state, qualifier TextId sequence, and base tag
TextId. It uses `definition_state_.struct_tag_def_map` only for TextId-less
legacy carriers. Focused lookup-authority coverage rejects stale parser-map
recovery plus same-TextId/same-context `struct_defs` candidates with mismatched
qualifier or global metadata, while direct complete `record_def` still wins.

## Suggested Next

Supervisor should review and commit this Step 4C template static-member/base
lookup slice with `src/frontend/parser/impl/types/template.cpp`,
`tests/frontend/frontend_parser_lookup_authority_tests.cpp`, this `todo.md`
update, and `test_after.log` if it is tracked in the local workflow. The
untracked review artifacts remain outside this packet.

## Watchouts

The static-member evaluator still passes `struct_tag_def_map` into
`eval_const_int` for static field initializer evaluation; this slice only
migrates final base record identity selection during recursive base traversal.
The new helper intentionally treats incomplete structured `record_def`
carriers with TextIds as structured misses unless the full applicable record
metadata matches a complete `struct_defs` candidate.

## Proof

Proof run:

`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(frontend_parser_tests|frontend_parser_lookup_authority_tests|llvm_gcc_c_torture_src_20000605_1_c|llvm_gcc_c_torture_src_20011008_3_c|llvm_gcc_c_torture_src_pr33870_1_c|llvm_gcc_c_torture_src_pr33870_c)$' > test_after.log`

Result: passed, 6/6 tests. Proof log: `test_after.log`.
