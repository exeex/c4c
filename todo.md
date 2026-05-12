Status: Active
Source Idea Path: ideas/open/200_hir_legacy_compatibility_retirement.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Fence Metadata-Rich Module And Owner Lookups

# Current Packet

## Just Finished

Completed `plan.md` Step 2 repair for module function/global declaration
lookup after the d479fec40 full-suite regressions. Complete structured misses
with mismatched TextIds still fail closed before rendered `fn_index` or
`global_index`, but same-TextId owner-metadata gaps can recover through the
explicit rendered compatibility path. Direct-call synthesized callees also keep
rendered compatibility after a structured miss so generated template-call
helpers can recover their semantic `LinkNameId`.

Focused lookup coverage in `tests/frontend/frontend_hir_lookup_tests.cpp` now
proves both same-TextId owner recovery and direct-call generated-callee
compatibility, while the existing stale rendered function/global tests still
reject mismatched complete structured misses. The HIR dump structured mirror
expectation records the repaired legacy owner recovery plus later
global-id/structured/link-name hits without parity mismatches.

## Suggested Next

Continue `plan.md` Step 2 with the next owner-lookup fence only after the
supervisor reviews whether same-TextId missing-owner compatibility should be
mirrored in record-owner helpers or kept module-decl specific.

## Watchouts

- Mismatched complete declaration-key misses remain fenced; the recovery added
  here only applies when the rendered compatibility target carries the same
  `TextId` as the `DeclRef`.
- Direct-call compatibility is deliberately narrower than general
  `resolve_function_decl`; strict declaration lookup still returns null for
  complete mismatched misses.
- The HIR dump regex now includes the same-TextId legacy owner recovery before
  later authoritative hits, while still rejecting parity mismatches.

## Proof

`bash -lc 'cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R "^(frontend_hir_lookup_tests|frontend_hir_tests|cpp_hir_module_decl_lookup_structured_mirror|cpp_positive_sema_anon_namespace_fn_lookup_cpp|cpp_positive_sema_namespace_global_var_runtime_cpp|cpp_positive_sema_namespace_reopen_runtime_cpp|cpp_positive_sema_rvalue_ref_param_basic_cpp|cpp_positive_sema_rvalue_ref_param_call_basic_cpp|cpp_positive_sema_tag_brace_forward_runtime_cpp)$"' > test_after.log 2>&1`

Result: passed. `test_after.log` is the proof log.
