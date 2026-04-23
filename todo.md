# Execution State

Status: Active
Source Idea Path: ideas/open/81_parser_state_convergence_and_scope_rationalization.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Regroup Parser Member Fields Into Explicit Bundles
# Current Packet

## Just Finished

- completed Step 2 by removing the pragma-state compatibility aliases from
  `Parser` and routing the owned parser pragma reads/writes directly through
  `pragma_state_` in `parser_core.cpp`, `parser_declarations.cpp`, and
  `parser_types_struct.cpp`

## Suggested Next

- continue Step 2 by checking for any remaining parser-bundle alias holdouts
  in the parser subsystem, then prove the same parser-focused subset again

## Watchouts

- keep the work inside the parser subsystem
- keep Step 2 focused on field layout and ownership readability, not helper
  rewrites or grammar changes
- preserve constructor, snapshot, and rollback behavior while the grouped
  layout is being converted to direct bundle access
- alias removals can reach non-parser call sites such as `src/apps/c4cll.cpp`,
  so audit repo-wide member reads and parser tests before assuming a family is
  parser-local

## Proof

- `cmake --build build -j --target c4c_frontend c4cll`
- `ctest --test-dir build -j --output-on-failure -R '^(frontend_parser_tests|cpp_parse_top_level_pragma_pack_preserves_following_decl_dump|cpp_parse_top_level_pragma_gcc_visibility_preserves_following_decl_dump)$'`
- Result: passed
- Log: `test_after.log`
