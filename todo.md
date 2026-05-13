Status: Active
Source Idea Path: ideas/open/200_hir_legacy_compatibility_retirement.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Audit HIR-To-LIR Handoff Compatibility Tags

# Current Packet

## Just Finished

Step 5 call handoff packet is complete. HIR-to-LIR call return, callee
signature, and call argument aggregate type-ref construction now stops before
rendered compatibility recovery when complete owner metadata is present but
misses. Call targets retain raw rendered text after the miss, argument mirrors
fail closed, and no-owner compatibility still produces structured
`StructNameId` mirrors.

## Suggested Next

Continue Step 5 by auditing the remaining HIR-to-LIR aggregate handoff paths
outside call return/signature/argument construction, especially va_arg,
const-init, lvalue/indexed-GEP, and field-chain helpers that still name
`legacy-compat` layout sites.

## Watchouts

- `typespec_aggregate_complete_owner_key_missed` is intentionally stricter
  than the cross-table compatibility owner helper. Use it only where complete
  owner misses must block rendered fallback.
- Call return mirrors may keep raw rendered text after a complete miss; call
  argument mirrors use an empty `LirTypeRef` so `arg_type_refs` is omitted while
  structured argument text remains available.
- Do not broaden this packet into verifier policy changes; the focused tests
  assert handoff construction behavior directly.

## Proof

Passed the delegated proof command:
`bash -lc 'cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R "^frontend_lir_(global_type_ref|function_signature_type_ref|extern_decl_type_ref|call_type_ref)$"' > test_after.log 2>&1`

Proof log: `test_after.log`
