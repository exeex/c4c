Status: Active
Source Idea Path: ideas/open/145_move_record_tag_authority_from_parser_to_sema.md
Source Plan Path: plan.md
Current Step ID: 4C
Current Step Title: Migrate Remaining Parser Record Lookup Families

# Current Packet

## Just Finished

Step 4C C declaration-completeness regression repair completed. C typedef
forward declarations such as `typedef struct Tag Alias; struct Tag { ... };
Alias value;` now complete visible typedef metadata from structured
`struct_defs` TextId/context carriers, not from rendered
`definition_state_.struct_tag_def_map` authority. The completed record is
written back to both the typedef binding and the declaration TypeSpec. Direct
complete `record_def` metadata remains accepted, and stale parser-map-only
structured misses still fail.

## Suggested Next

Supervisor should review and commit this Step 4C regression-repair slice with
`src/frontend/parser/impl/declarations.cpp`,
`tests/frontend/frontend_parser_tests.cpp`, this `todo.md` update, and
`test_after.log` if it is tracked in the local workflow. The untracked review
artifacts remain outside this packet.

## Watchouts

The repair deliberately scans `definition_state_.struct_defs`, not
`struct_tag_def_map`, and requires matching `unqualified_text_id`,
namespace context when present, and struct/union kind. It is limited to C mode
typedef completion for declaration checks and does not change record layout
constant evaluation or other parser-map consumers.

## Proof

Proof run:

`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(frontend_parser_tests|frontend_parser_lookup_authority_tests|llvm_gcc_c_torture_src_20000605_1_c|llvm_gcc_c_torture_src_20011008_3_c|llvm_gcc_c_torture_src_20040703_1_c|llvm_gcc_c_torture_src_pr33870_1_c|llvm_gcc_c_torture_src_pr33870_c)$' > test_after.log`

Result: passed, 7/7 tests. Proof log: `test_after.log`.

Supervisor full-suite validation:

`ctest --test-dir build -j --output-on-failure`

Result: passed, 3023/3023 tests.
