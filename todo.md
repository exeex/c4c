Status: Active
Source Idea Path: ideas/open/200_hir_legacy_compatibility_retirement.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Audit HIR-To-LIR Handoff Compatibility Tags

# Current Packet

## Just Finished

Step 5 call handoff packet is complete. HIR-to-LIR call return, callee
signature, call argument aggregate type-ref construction, and the remaining
layout handoff sites now share the Step 5 stale rendered owner-miss fence.
Const-init, va_arg, variadic aggregate args, indexed-GEP, field-chain, member
access, aggregate value-type, and size/layout lookup paths fail closed after
complete owner-key misses. No-owner rendered compatibility remains available,
and exact materialized-template layout carriers remain accepted through
`typespec_aggregate_complete_owner_key_missed`.

## Suggested Next

Supervisor should decide whether Step 5 is accepted and hand off to Step 6 for
the final ledger and broader validation checkpoint.

## Watchouts

- The shared `lookup_structured_layout` fence is intentionally duplicated at
  the compatibility-decl and structured-name-id helpers so direct helper calls
  cannot bypass the Step 5 complete-miss rule.
- `field_chain_nested_aggregate` still has a no-metadata legacy compatibility
  route for anonymous aggregate members, but only after the child TypeSpec is
  not a complete owner-key miss.
- `vaarg_aggregate_structured_name_id` now uses the cross-table-aware owner
  helper; exact materialized template tags remain valid structured authority.

## Proof

Passed the delegated proof command:
`bash -lc 'cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R "^(frontend_lir_(global_type_ref|function_signature_type_ref|extern_decl_type_ref|call_type_ref)|frontend_hir_lookup_tests)$"' > test_after.log 2>&1`

Proof log: `test_after.log`
