Status: Active
Source Idea Path: ideas/open/200_hir_legacy_compatibility_retirement.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Fence Consteval Replay And Pending Identity Names

# Current Packet

## Just Finished

Completed `plan.md` Step 3. Audited HIR consteval replay and pending identity
lookup around `PendingConstevalExpr::fn_name`, `pending_consteval_key`, and
`find_pending_consteval_def`. Complete `callee_identity` paths already fail
closed through the structured consteval registry before rendered-name lookup,
and the remaining rendered `fn_name` fallback is now documented as
display/no-metadata compatibility only.

Confirmed existing focused stale-name coverage:
`test_consteval_call_lowering_rejects_stale_rendered_registry_after_metadata_miss`
and
`test_pending_consteval_replay_rejects_stale_rendered_registry_after_metadata_miss`.

## Suggested Next

Proceed to the next supervisor-selected packet after Step 3 review. A coherent
next packet is to continue the legacy compatibility retirement sweep at the next
plan step, using the same pattern: identify one rendered fallback surface,
fence complete structured misses, and preserve no-metadata compatibility only
where explicitly needed.

## Watchouts

- `PendingConstevalExpr::fn_name` is still used for diagnostics,
  `ConstevalCallInfo` display metadata, and no-metadata replay compatibility.
  It must not become semantic authority after a complete `callee_identity` miss.
- `evaluate_consteval_call` still receives rendered consteval maps for nested
  no-metadata compatibility, with structured/text mirrors supplied alongside
  them.

## Proof

`bash -lc 'cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R "^(frontend_hir_lookup_tests|frontend_hir_tests|cpp_hir_consteval_reduction_verified|cpp_hir_consteval_template_reduction_verified|cpp_hir_deferred_consteval_(chain|multi|incomplete_type)|cpp_positive_sema_deferred_consteval_(chain|multi|incomplete_type|nttp)_cpp|cpp_positive_sema_consteval_)"' > test_after.log 2>&1`

Result: passed. `test_after.log` is the proof log.
