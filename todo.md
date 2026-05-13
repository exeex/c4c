Status: Active
Source Idea Path: ideas/open/200_hir_legacy_compatibility_retirement.md
Source Plan Path: plan.md
Current Step ID: 6
Current Step Title: Final Ledger And Broader Validation

# Current Packet

## Just Finished

Step 5 is complete. HIR-to-LIR call return, callee signature, call argument
aggregate type-ref construction, and the remaining layout handoff sites now
share the Step 5 stale rendered owner-miss fence. Const-init, va_arg,
variadic aggregate args, indexed-GEP, field-chain, member access, aggregate
value-type, and size/layout lookup paths fail closed after complete owner-key
misses. No-owner rendered compatibility remains available, exact materialized
template layout carriers remain accepted through
`typespec_aggregate_complete_owner_key_missed`, and the accepted full-suite
baseline candidate was 3137/3137.

## Suggested Next

Begin Step 6. Produce the final HIR compatibility retirement ledger, confirm
the remaining rendered maps are display/order, diagnostics, generated
route-local state, final output spelling, or explicit no-metadata
compatibility, and run the supervisor-selected broader validation checkpoint.

## Watchouts

- The shared `lookup_structured_layout` fence is intentionally duplicated at
  the compatibility-decl and structured-name-id helpers so direct helper calls
  cannot bypass the Step 5 complete-miss rule.
- `field_chain_nested_aggregate` still has a no-metadata legacy compatibility
  route for anonymous aggregate members, but only after the child TypeSpec is
  not a complete owner-key miss.
- `vaarg_aggregate_structured_name_id` now uses the cross-table-aware owner
  helper; exact materialized template tags remain valid structured authority.
- Step 6 should record whether HIR-to-LIR and frontend-to-BIR handoff may
  proceed without new rendered-name recovery fallbacks; do not expand scope
  beyond the active HIR compatibility retirement source idea.

## Proof

Step 5 passed the delegated proof command:
`bash -lc 'cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R "^(frontend_lir_(global_type_ref|function_signature_type_ref|extern_decl_type_ref|call_type_ref)|frontend_hir_lookup_tests)$"' > test_after.log 2>&1`

Accepted full-suite baseline candidate: 3137/3137.

Proof log: `test_after.log`
