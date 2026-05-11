Status: Active
Source Idea Path: ideas/open/166_hir_rendered_registry_mirror_retirement_audit.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Process Remaining Lowerer Registries

# Current Packet

## Just Finished

Completed plan Step 5 lowerer static-member const-value regression repair:
complete owner/member misses still reject stale rendered
`struct_static_member_const_values_`, but generated template helper
instantiations now try the selected specialization pattern and its materialized
template base before walking rendered base tags. This restores the
`is_signed_helper<T, false>` const-value path without reopening stale rendered
collisions.

## Suggested Next

Supervisor can review and commit this Step 5 static-member const-value
compatibility repair, then choose the next remaining lowerer registry packet or
lifecycle handoff.

## Watchouts

The selected-pattern base materialization is intentionally limited to generated
template instantiations whose selected pattern is not the primary. Primary
patterns keep their existing rendered/generated base path so dependent base
expressions such as signedness comparisons are not collapsed through default
NTTP values.

## Proof

Ran the delegated proof command:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(frontend_hir_tests|cpp_positive_sema_eastl_type_traits_signed_helper_base_expr_parse_cpp)$' > test_after.log 2>&1`.

Result: passed. `test_after.log` contains the passing focused
`frontend_hir_tests` stale rendered static-member const-value coverage and the
passing `cpp_positive_sema_eastl_type_traits_signed_helper_base_expr_parse_cpp`
runtime regression case.
