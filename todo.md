# Execution State

Status: Active
Source Idea Path: ideas/open/81_parser_state_convergence_and_scope_rationalization.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Regroup Parser Member Fields Into Explicit Bundles
# Current Packet

## Just Finished

- completed the Step 2 alias-removal packet by deleting the `Parser`
  `token_mutations_` compatibility alias and routing the affected template
  token-splitting helpers directly through `core_input_state_`

## Suggested Next

- continue Step 2 with the next parser bundle alias-removal family, with the
  remaining core-input aliases (`tokens_`, `pos_`, and `arena_`) as the next
  narrow candidate now that token mutation rollback uses `core_input_state_`
  directly

## Watchouts

- keep the work inside the parser subsystem
- keep Step 2 focused on field layout and ownership readability, not helper
  rewrites or grammar changes
- preserve constructor, snapshot, and rollback behavior while the grouped
  layout is being converted to direct bundle access
- the token-mutation rollback path now uses `core_input_state_`; if the next
  packet continues core-input alias removal, keep tentative and token-splitting
  behavior identical while moving `tokens_`, `pos_`, or `arena_` reads
- the proof passed and is captured in `test_after.log`

## Proof

- `cmake --build build -j && ctest --test-dir build -j --output-on-failure -R '^(frontend_parser_tests|cpp_parse_top_level_pragma_pack_preserves_following_decl_dump|cpp_parse_top_level_pragma_gcc_visibility_preserves_following_decl_dump)$'`
- Result: passed
- Log: `test_after.log`
