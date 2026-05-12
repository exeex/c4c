Status: Active
Source Idea Path: ideas/open/199_sema_legacy_compatibility_retirement.md
Source Plan Path: plan.md
Current Step ID: 6
Current Step Title: Closure Ledger And Broader Sema Proof

# Current Packet

## Just Finished

Step 6 closure ledger is recorded for the Sema legacy compatibility retirement
runbook.

- Deleted routes: no production Sema compatibility route was fully deleted in
  this runbook because each rendered bridge still had an explicit no-metadata,
  diagnostic/display, source-payload, final-spelling, or HIR-boundary caller.
- Converted authority: metadata-rich static-eval enum, consteval type-binding,
  NTTP, consteval function, interpreter local-const, and struct-def/layout
  handoff lookups now use structured key/TextId/owner metadata as semantic
  authority. Complete structured misses fail closed before rendered maps can
  recover stale names.
- Fenced compatibility: retained rendered enum lookup, rendered
  `same_rendered_type_name_compatibility`, rendered `TypeBindings` fallback,
  forwarded/default NTTP rendered mirrors, rendered `consteval_fns`, rendered
  local-const lookup, interpreter `by_name`, and rendered
  `ConstEvalEnv::struct_defs` handoff are documented as legacy or deprecated
  no-metadata compatibility with owner, limitation, and removal-condition
  notes.
- Intentionally retained routes: rendered names remain valid as display text,
  diagnostics, source payload, ABI/final output spelling, direct no-metadata
  opt-in bridges, and HIR/static-member boundary compatibility where upstream
  callers do not yet provide complete structured metadata.
- Proof coverage: focused packets covered structured success,
  stale-rendered fail-closed behavior, and retained no-metadata compatibility
  for enum, type-binding, NTTP, consteval function, local const, and
  struct-def/layout routes.
- Residual follow-up scope: parser syntax-carrier cleanup, broad HIR registry
  or lowerer map cleanup, BIR, LIR, backend compatibility, and final output
  spelling work remain separate initiatives and were not hidden inside this
  Sema-owned runbook.

## Suggested Next

Return to the supervisor for Step 6 acceptance review and lifecycle decision.
No further executor packet is required unless the broader proof fails or review
finds a missing ledger item.

## Watchouts

- The ledger is intentionally Sema-owned. Parser, HIR, BIR, LIR, backend, and
  final-spelling cleanup should be planned separately if needed.
- Retained rendered bridges must stay no-metadata/display/source/final-spelling
  only. Future metadata-rich misses must keep failing closed before rendered
  recovery.
- In this frontend TypeSpec shape there is no legacy `tag` field, so existing
  proofs use key/TextId/owner metadata rather than fabricated rendered-tag
  carriers.

## Proof

Passed delegated proof:
`bash -lc 'cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R "^(frontend_parser_tests|cpp_hir_.*structured_metadata|cpp_positive_sema_)"' > test_after.log 2>&1`

Result: passed, 923/923 tests green. Proof log: `test_after.log`.
