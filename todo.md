# Execution State

Status: Active
Source Idea Path: ideas/open/81_parser_state_convergence_and_scope_rationalization.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Regroup Parser Member Fields Into Explicit Bundles
# Current Packet

## Just Finished

- completed the Step 2 `parser_expressions.cpp` bundle-routing packet by
  moving read-only cursor and token-window probes onto `core_input_state_`
  access for `sizeof...` text capture, delete/new-array operator lookahead,
  qualified `operator` name probing, balanced-template-id lookahead, GCC
  type-trait builtin lookahead, and global-qualified `::new` / `::identifier`
  checks while leaving explicit `pos_` / `tokens_` save-restore mechanics
  unchanged for later Step 3 classification

## Suggested Next

- move the next Step 2 bundle-routing packet to another parser implementation
  file with read-only `pos_` / `tokens_` layout probes, likely
  `parser_types_struct.cpp` or the remaining read-only windows in
  `parser_types_base.cpp`, while continuing to leave real save/restore and
  injected-token mechanics for Step 3

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
- the remaining direct `pos_` / `tokens_` references in
  `parser_expressions.cpp` are concentrated in intentional save/restore,
  rollback, or token-copy sites that should stay in place for this Step 2
  slice

## Proof

- `cmake --build build -j && ctest --test-dir build -j --output-on-failure -R '^(frontend_parser_tests|cpp_parse_top_level_pragma_pack_preserves_following_decl_dump|cpp_parse_top_level_pragma_gcc_visibility_preserves_following_decl_dump)$' | tee test_after.log`
- Result: passed; 3/3 tests passed in the delegated subset
- Log: `test_after.log`
