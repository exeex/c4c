Status: Active
Source Idea Path: ideas/open/199_sema_legacy_compatibility_retirement.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Audit Struct-Def And Layout Handoff Compatibility

# Current Packet

## Just Finished

Step 4 consteval-function rendered lookup fencing is complete for this packet.
The retained `consteval_fns` rendered fallback in the consteval API and
`lookup_consteval_function` now has nearby legacy/deprecated owner, limitation,
and removal-condition notes. Behavior is unchanged: TextId/key function maps
remain authoritative for metadata-rich calls, metadata misses cannot recover
through rendered names, and no-metadata recursive/chained consteval calls keep
the rendered compatibility bridge.

## Suggested Next

Execute Step 5: audit rendered `ConstEvalEnv::struct_defs` and any
Sema-facing layout or struct-def compatibility helpers discovered during
inventory. Prefer structured record/layout identity for metadata-rich callers,
fence any retained rendered struct-def lookup as legacy/no-metadata handoff
compatibility, and add focused stale rendered layout or struct-def proof.

## Watchouts

- The rendered enum bridge remains intentionally retained because HIR/static-
  member no-metadata callers still opt in through the named compatibility API.
- Type-binding and NTTP rendered names may remain as display/source payload or
  explicit no-metadata compatibility; they must not act as semantic authority
  after a complete structured miss.
- Step 5 should stay Sema-owned. Do not broaden into parser, HIR lowerer, BIR,
  LIR, or backend cleanup unless the supervisor assigns that scope.
- Retained rendered struct-def/layout handoff must have nearby legacy or
  deprecated owner, limitation, and removal-condition notes.
- A metadata-rich layout miss must not recover through rendered
  `ConstEvalEnv::struct_defs` compatibility.
- Retained local-const rendered lookup now has the requested nearby
  legacy/deprecated notes; future edits should preserve the invariant that a
  local TextId/key metadata miss sets `skip_local` and cannot fall through to
  `local_consts` or interpreter `by_name`.
- The retained `consteval_fns` rendered fallback is intentionally no-metadata
  only; a metadata-rich miss must not fall through to it.

## Proof

Passed delegated proof:
`bash -lc 'cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R "^(frontend_parser_tests|cpp_hir_sema_consteval_type_utils_structured_metadata)$"' > test_after.log 2>&1`

Result: passed, 2/2 focused tests green. Proof log: `test_after.log`.
