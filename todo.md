# Execution State

Status: Active
Source Idea Path: ideas/open/81_parser_state_convergence_and_scope_rationalization.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Regroup Parser Member Fields Into Explicit Bundles


# Current Packet

## Just Finished

- completed the Step 2 `parser_types_declarator.cpp` probe-routing packet by
  moving the read-only template/declarator lookahead helpers onto
  `core_input_state_` access while leaving the intentional token-stream
  save/restore rollback sites unchanged

## Suggested Next

- continue Step 2 in `parser_types_declarator.cpp` by routing the remaining
  read-only cursor probes and token-spelling helpers through
  `core_input_state_`, while still avoiding the injected-token save/restore
  path in the dependent-typename flow

## Watchouts

- keep the work inside the parser subsystem
- keep Step 2 focused on field layout and ownership readability, not helper
  rewrites or grammar changes
- preserve constructor, snapshot, and rollback behavior while the grouped
  layout is being converted to direct bundle access
- keep future bundle-routing work behavior-preserving around cursor movement,
  diagnostics, and rollback, especially where tentative parsing still performs
  explicit token-stream save/restore
- treat the remaining direct `pos_` save/restore sites in
  `parser_types_declarator.cpp` as intentional rollback mechanics unless a
  later Step 3 classification packet chooses to rework them
- keep the dependent-typename injected-token detour intact for this Step 2
  route; it is not just a read-only probe
- refresh proof after each bounded bundle-routing packet and keep it captured
  in `test_after.log`

## Proof

- `cmake --build build -j && ctest --test-dir build -j --output-on-failure -R '^(frontend_parser_tests|cpp_parse_top_level_pragma_pack_preserves_following_decl_dump|cpp_parse_top_level_pragma_gcc_visibility_preserves_following_decl_dump)$' | tee test_after.log`
- Result: passed
- Log: `test_after.log`
