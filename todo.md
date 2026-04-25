Status: Active
Source Idea Path: ideas/open/99_hir_module_symbol_structured_lookup_mirror.md
Source Plan Path: plan.md
Current Step ID: 9
Current Step Title: Audit and remediate legacy fallback readiness

# Current Packet

## Just Finished

Step 8: Add targeted lookup proof/instrumentation. Reviewer accepted the Step 8
proof as aligned and non-overfit: authority classification now distinguishes
structured, legacy rendered, concrete global-ID, and `LinkNameId` module
function/global hits without changing resolver precedence.

## Suggested Next

Step 9: Audit and remediate legacy fallback readiness. Enumerate remaining
legacy-rendered hits and declaration/qualifier metadata gaps, remediate narrow
behavior-preserving metadata gaps where existing AST/HIR metadata is already
available, and leave fallback precedence unchanged.

## Watchouts

- Preserve rendered-name maps and link-name behavior.
- Hoisted compound-literal globals still have no declaration metadata and keep
  the default invalid `name_text_id`, so they update only the rendered global
  map through `Module::index_global_decl`.
- Structured lookup currently requires complete text-id metadata for all
  qualifier segments; refs with incomplete metadata intentionally fall back to
  legacy rendered-name lookup.
- Parity mismatch visibility is HIR-dump scoped and only emits when a resolver
  call has observed a structured/rendered disagreement.
- The new focused case proves no parity-mismatch report is emitted for the
  namespace-qualified fixture; it does not alter implementation behavior.
- Do not demote rendered fallback during Step 8; this step is evidence
  gathering only.
- The reviewer specifically noted that resolver parity currently runs after
  concrete-ID and `LinkNameId` bridge paths, so targeted proof must make those
  paths distinguishable from structured and legacy rendered hits.
- Step 8 now explicitly proves legacy-rendered lookup hits with a direct
  frontend-only HIR `Module` resolver test, alongside structured,
  concrete global-ID, and `LinkNameId` hit coverage.
- Reviewer recommendation from
  `review/99_step8_lookup_authority_proof_review.md`: keep direct fallback
  demotion parked. Step 9 should begin as an audit/remediation readiness
  packet that enumerates remaining legacy-rendered hits and metadata gaps.
- Do not change fallback precedence, remove rendered lookup, or start Step 10
  demotion without explicit supervisor approval.
- Do not broaden into struct/type, member/method, parser, or unrelated sema
  rewrites.
- Do not downgrade expectations or add testcase-shaped shortcuts.

## Proof

Passed. Proof output is captured in `test_after.log`.

- `cmake --build --preset default && ctest --test-dir build -j --output-on-failure -L "^hir$"`
