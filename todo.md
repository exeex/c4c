Status: Active
Source Idea Path: ideas/open/99_hir_module_symbol_structured_lookup_mirror.md
Source Plan Path: plan.md
Current Step ID: 9
Current Step Title: Audit and remediate legacy fallback readiness

# Current Packet

## Just Finished

Step 9: Audit and remediate legacy fallback readiness. The focused HIR dump
fixture has no real-lowering legacy-rendered hits: it resolves the namespace
function through structured lookup, globals through concrete `GlobalId`, and
the call through `LinkNameId`. The direct HIR lookup test now explicitly
classifies the remaining compatibility fallback cases as missing declaration
metadata and incomplete qualifier metadata for both functions and globals.
No behavior-preserving implementation remediation was available from existing
AST/HIR metadata, so fallback precedence remains unchanged. Audit details are
recorded in `review/99_step9_legacy_fallback_readiness.md`.

## Suggested Next

Supervisor should review the Step 9 audit and decide whether to explicitly
approve a narrow Step 10 demotion packet or leave rendered fallback parked.

## Watchouts

- Preserve rendered-name maps and link-name behavior.
- Hoisted compound-literal globals still have no declaration metadata and keep
  the default invalid `name_text_id`, so they update only the rendered global
  map through `Module::index_global_decl`; no focused module lookup hit currently
  depends on demoting that case.
- Structured lookup requires complete text-id metadata for all qualifier
  segments; refs with incomplete metadata intentionally fall back to legacy
  rendered-name lookup.
- Parity mismatch visibility is HIR-dump scoped and only emits when a resolver
  call has observed a structured/rendered disagreement.
- The focused dump fixture still proves no parity-mismatch report is emitted for
  the namespace-qualified path.
- Do not change fallback precedence, remove rendered lookup, or start Step 10
  demotion without explicit supervisor approval.
- Do not broaden into struct/type, member/method, parser, or unrelated sema
  rewrites.
- Do not downgrade expectations or add testcase-shaped shortcuts.

## Proof

Passed. Proof output is captured in `test_after.log`.

- `cmake --build --preset default && ctest --test-dir build -j --output-on-failure -L "^hir$"`
