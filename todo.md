# Execution State

Status: Active
Source Idea Path: ideas/open/81_parser_state_convergence_and_scope_rationalization.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Regroup Parser Member Fields Into Explicit Bundles
# Current Packet

## Just Finished

- completed the Step 2 `parser_types_template.cpp` packet by routing its
  remaining `tokens_`, `pos_`, and `arena_` reads directly through
  `core_input_state_`

## Suggested Next

- continue Step 2 with the next narrow core-input alias-removal family in
  `parser_core.cpp`, starting with the token cursor helpers and nearby
  file/position diagnostics that still read `tokens_` and `pos_` directly

## Watchouts

- keep the work inside the parser subsystem
- keep Step 2 focused on field layout and ownership readability, not helper
  rewrites or grammar changes
- preserve constructor, snapshot, and rollback behavior while the grouped
  layout is being converted to direct bundle access
- the injected-template parse helper now swaps `core_input_state_.tokens` and
  `core_input_state_.pos` directly; keep future core-input alias removals
  behavior-preserving around cursor movement, diagnostics, and rollback
- the proof passed and is captured in `test_after.log`

## Proof

- `cmake --build build -j && ctest --test-dir build -j --output-on-failure -R '^(frontend_parser_tests|cpp_parse_top_level_pragma_pack_preserves_following_decl_dump|cpp_parse_top_level_pragma_gcc_visibility_preserves_following_decl_dump)$'`
- Result: passed
- Log: `test_after.log`
