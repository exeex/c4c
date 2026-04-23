# Execution State

Status: Active
Source Idea Path: ideas/open/81_parser_state_convergence_and_scope_rationalization.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Regroup Parser Member Fields Into Explicit Bundles
# Current Packet

## Just Finished

- completed the Step 2 `parser_core.cpp` packet by routing the token cursor
  helpers, tentative-parse debug hooks, and nearby diagnostic formatting paths
  through `core_input_state_` instead of the direct `tokens_`/`pos_` aliases

## Suggested Next

- continue Step 2 in `parser_core.cpp` with the remaining qualified-name and
  token-mutation helper reads that still touch `tokens_` and `pos_` directly

## Watchouts

- keep the work inside the parser subsystem
- keep Step 2 focused on field layout and ownership readability, not helper
  rewrites or grammar changes
- preserve constructor, snapshot, and rollback behavior while the grouped
  layout is being converted to direct bundle access
- the injected-template parse helper now swaps `core_input_state_.tokens` and
  `core_input_state_.pos` directly; keep future core-input alias removals
  behavior-preserving around cursor movement, diagnostics, and rollback
- the remaining `parser_core.cpp` hits are in the qualified-name helper and
  token-mutation paths; keep them as the next bounded packet instead of
  pulling in broader parser logic
- the proof passed and is captured in `test_after.log`

## Proof

- `cmake --build build -j && ctest --test-dir build -j --output-on-failure -R '^(frontend_parser_tests|cpp_parse_top_level_pragma_pack_preserves_following_decl_dump|cpp_parse_top_level_pragma_gcc_visibility_preserves_following_decl_dump)$' | tee test_after.log`
- Result: passed
- Log: `test_after.log`
