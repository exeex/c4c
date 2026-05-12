Status: Active
Source Idea Path: ideas/open/200_hir_legacy_compatibility_retirement.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Fence Consteval Replay And Pending Identity Names

# Current Packet

## Just Finished

Repaired the prior Step 2 value-args owner recovery fence while keeping Step 3
as the active plan step. `resolve_member_lookup_owner_tag` now allows concrete
materialized template record tags to remain member-owner carriers, but complete
qualified owner-key misses still reject stale rendered tag recovery.

`try_eval_template_static_member_const` now resolves complete qualified
template-owner misses only through the structured qualified primary
(`ns::is_signed`) before its stale-rendered-primary fence, preserves
no-metadata compatibility, and carries materialized template owners through
deferred member typedef resolution for value-arg static-member traits.

Added focused lookup coverage for the materialized-tag carrier case while the
existing stale rendered-owner tests continue to pass.

## Suggested Next

Proceed to the next supervisor-selected packet. A coherent next packet is to
continue the legacy compatibility retirement sweep at the next plan step, using
the same pattern: identify one rendered fallback surface, fence complete
structured misses, and preserve no-metadata or structured materialization
compatibility only where explicitly needed.

## Watchouts

- The value-args repair intentionally does not allow a complete qualified owner
  key miss such as `MissingNs::RealOwner` to fall through to an unqualified
  rendered primary. The structured qualified primary must exist.
- Unqualified materialized tags such as `Box_T_struct_Pair_T_int` are concrete
  HIR record carriers, not declaration-owner rendered fallback authority.

## Proof

`bash -lc 'cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R "^(frontend_hir_lookup_tests|frontend_hir_tests|cpp_hir_value_args_residual_structured_metadata|cpp_hir_template_value_arg_static_member_trait|cpp_hir_template_deferred_nttp_(expr|arith_expr|paren_expr|bool_expr|logic_expr|true_expr|number_expr|static_member_expr|cast_static_member_expr|sizeof_pack_expr)|cpp_hir_template_alias_deferred_nttp_static_member|cpp_positive_sema_template_angle_bracket_validation_cpp|cpp_positive_sema_template_struct_advanced_cpp|cpp_positive_sema_template_struct_nested_cpp|cpp_positive_sema_template_variable_member_typedef_normalization_runtime_cpp|eastl_cpp_external_utility_frontend_basic_cpp)$"' > test_after.log 2>&1`

Result: passed. `test_after.log` is the proof log.
