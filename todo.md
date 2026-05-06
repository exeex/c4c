Status: Active
Source Idea Path: ideas/open/145_move_record_tag_authority_from_parser_to_sema.md
Source Plan Path: plan.md
Current Step ID: 4C
Current Step Title: Migrate Remaining Parser Record Lookup Families

# Current Packet

## Just Finished

Step 4C static-member initializer migration completed for
`Parser::eval_deferred_nttp_expr_tokens`. Static field and child initializer
constant evaluation now withholds `definition_state_.struct_tag_def_map` unless
the initializer's record-layout queries are explicitly limited to TextId-less
legacy carriers; structured record carriers use direct complete `record_def`
layout or the existing unresolved-layout fallback without parser-map recovery.
Focused lookup-authority coverage drives `Trait<int>::value` through the
deferred NTTP static-member path and distinguishes stale parser-map layout from
direct complete `record_def` layout.

## Suggested Next

Supervisor should review and commit this Step 4C static-member initializer
lookup slice with `src/frontend/parser/impl/types/template.cpp`,
`tests/frontend/frontend_parser_lookup_authority_tests.cpp`, this `todo.md`
update, and `test_after.log` if it is tracked in the local workflow. The
untracked review artifacts remain outside this packet.

## Watchouts

`eval_const_int` still returns its conservative unresolved struct size when a
structured layout carrier is incomplete; this slice only ensures stale parser
map entries cannot provide that layout authority from the template
static-member initializer call site. TextId-less legacy initializer carriers
still receive the compatibility map.

## Proof

Proof run:

`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(frontend_parser_tests|frontend_parser_lookup_authority_tests|llvm_gcc_c_torture_src_20000605_1_c|llvm_gcc_c_torture_src_20011008_3_c|llvm_gcc_c_torture_src_pr33870_1_c|llvm_gcc_c_torture_src_pr33870_c)$' > test_after.log`

Result: passed, 6/6 tests. Proof log: `test_after.log`.
