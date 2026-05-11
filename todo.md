Status: Active
Source Idea Path: ideas/open/166_hir_rendered_registry_mirror_retirement_audit.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Process Remaining Module and Compile-Time Registries

# Current Packet

## Just Finished

Completed plan Step 4 regression repair for Module declaration lookup fencing:
complete structured declaration-key misses still fail closed, while rendered
fallback remains available for self-consistent legacy decl refs and rendered
qualified compatibility refs that lack structured qualifier metadata. This
restores namespace/global/direct-call/rvalue-ref compatibility without
reopening stale rendered `fn_index`/`global_index` hits.

## Suggested Next

Supervisor can review and commit this Step 4 regression repair, then decide
whether any remaining compile-time registry handoff is still needed.

## Watchouts

The compatibility allowance is intentionally narrow: rendered fallback after a
structured miss is allowed only when the rendered name is TextId-consistent or
when the rendered name is qualified but the DeclRef has no structured
qualifier metadata. Stale rendered-name collisions with a different complete
source declaration TextId remain fail-closed.

## Proof

Ran the delegated proof command:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(frontend_hir_tests|frontend_hir_lookup_tests|cpp_hir_module_decl_lookup_structured_mirror|cpp_positive_sema_anon_namespace_fn_lookup_cpp|cpp_positive_sema_namespace_global_var_runtime_cpp|cpp_positive_sema_namespace_reopen_runtime_cpp|cpp_positive_sema_rvalue_ref_param_basic_cpp|cpp_positive_sema_rvalue_ref_param_call_basic_cpp)$' > test_after.log 2>&1`.

Result: passed. `test_after.log` contains the passing focused eight-test ctest
run.
