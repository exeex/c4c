# Execution State

Status: Active
Source Idea Path: ideas/open/81_parser_state_convergence_and_scope_rationalization.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Regroup Parser Member Fields Into Explicit Bundles
# Current Packet

## Just Finished

- completed the Step 2 `parser_statements.cpp` bundle-routing packet by moving
  read-only cursor and token-window reads onto `core_input_state_` access
  while leaving the intentional save/restore rollback sites on `pos_`
  unchanged

## Suggested Next

- move the next Step 2 bundle-routing packet to the next parser implementation
  file that still has read-only `pos_`/`tokens_` layout reads, and keep any
  remaining direct `pos_` sites that are true rollback mechanics for later
  Step 3 classification

## Watchouts

- keep the work inside the parser subsystem
- keep Step 2 focused on field layout and ownership readability, not helper
  rewrites or grammar changes
- preserve constructor, snapshot, and rollback behavior while the grouped
  layout is being converted to direct bundle access
- keep future bundle-routing work behavior-preserving around cursor movement,
  diagnostics, and rollback, especially where tentative parsing still performs
  explicit token-stream save/restore
- refresh proof after each bounded bundle-routing packet and keep it captured
  in `test_after.log`
- the remaining direct `pos_` references in `parser_statements.cpp` are the
  intentional save/restore mechanics and should stay in place for this Step 2
  slice

## Proof

- `cmake --build build -j && ctest --test-dir build -j --output-on-failure -R '^(frontend_parser_tests|cpp_parse_top_level_pragma_pack_preserves_following_decl_dump|cpp_parse_top_level_pragma_gcc_visibility_preserves_following_decl_dump)$' | tee test_after.log`
- Result: passed; 3/3 tests passed in the delegated subset
- Log: `test_after.log`
