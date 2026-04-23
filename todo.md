# Execution State

Status: Active
Source Idea Path: ideas/open/81_parser_state_convergence_and_scope_rationalization.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Regroup Parser Member Fields Into Explicit Bundles
# Current Packet

## Just Finished

- completed the Step 2 `parser_types_struct.cpp` bundle-routing packet by
  moving the read-only record-access, member-lookahead, constructor/destructor
  probe, specialization lookahead, and parameter attribute windows onto
  `core_input_state_` access while leaving the intentional save/restore and
  rollback paths unchanged

## Suggested Next

- move the next Step 2 bundle-routing packet to the remaining parser
  implementation file with read-only `pos_` / `tokens_` layout probes, likely
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
  `parser_types_struct.cpp` are intentional save/restore or rollback sites and
  should stay in place for this Step 2 slice

## Proof

- `cmake --build build -j && ctest --test-dir build -j --output-on-failure -R '^(frontend_parser_tests|cpp_positive_sema_access_labels_parse_cpp|cpp_positive_sema_access_labels_treated_public_runtime_cpp|cpp_positive_sema_friend_access_parse_cpp|cpp_positive_sema_keyword_friend_parse_cpp|cpp_positive_sema_class_specific_new_delete_parse_cpp|cpp_positive_sema_record_body_context_parse_cpp|cpp_positive_sema_record_body_context_teardown_parse_cpp|cpp_positive_sema_record_member_dispatch_parse_cpp|cpp_positive_sema_record_member_entry_parse_cpp|cpp_positive_sema_record_member_method_field_parse_cpp|cpp_positive_sema_record_member_special_dispatch_parse_cpp|cpp_positive_sema_record_member_special_member_parse_cpp|cpp_positive_sema_record_member_type_dispatch_parse_cpp)$' | tee test_after.log`
- Result: passed; 14/14 tests passed in the delegated subset
- Log: `test_after.log`
