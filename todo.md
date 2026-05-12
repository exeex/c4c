Status: Active
Source Idea Path: ideas/open/199_sema_legacy_compatibility_retirement.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Audit Struct-Def And Layout Handoff Compatibility

# Current Packet

## Just Finished

Step 5 struct-def/layout handoff fencing is complete for this packet.
`lookup_record_layout` now keeps the retained bare rendered
`ConstEvalEnv::struct_defs` lookup fenced to legacy/no-metadata handoff: if a
record TypeSpec carries complete owner metadata, a layout miss cannot recover
through rendered `struct_defs`, even when no `struct_def_owner_index` was
provided. The focused consteval metadata test now covers stale rendered
`sizeof`/`alignof` recovery with no owner index, and the existing parser/HIR
layout proofs remain green.

## Suggested Next

Return to the supervisor for Step 5 acceptance review and the next lifecycle
decision. No further executor packet is required for this Step 5 slice unless
review asks for broader validation or an additional stale-layout harness.

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
  `ConstEvalEnv::struct_defs` compatibility, including the no-owner-index
  handoff edge.
- Retained local-const rendered lookup now has the requested nearby
  legacy/deprecated notes; future edits should preserve the invariant that a
  local TextId/key metadata miss sets `skip_local` and cannot fall through to
  `local_consts` or interpreter `by_name`.
- The retained `consteval_fns` rendered fallback is intentionally no-metadata
  only; a metadata-rich miss must not fall through to it.
- In this frontend TypeSpec shape there is no legacy `tag` field, so the new
  consteval unit proof focuses on rejecting metadata-rich stale rendered layout
  recovery rather than fabricating a positive rendered-tag layout carrier.

## Proof

Passed delegated proof:
`bash -lc 'cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R "^(frontend_parser_tests|cpp_hir_sema_consteval_type_utils_structured_metadata|cpp_hir_builtin_layout_query_(sizeof_type|alignof_type|alignof_expr))$"' > test_after.log 2>&1`

Result: passed, 5/5 focused tests green. Proof log: `test_after.log`.
