Status: Active
Source Idea Path: ideas/open/168_compatibility_bridge_retirement.md
Source Plan Path: plan.md
Current Step ID: 7
Current Step Title: Consolidate Retained Bridges and Final Proof

# Current Packet

## Just Finished

Step 7 - Consolidate Retained Bridges and Final Proof has been recorded as a
todo-only lifecycle consolidation packet. The active runbook remains open for
supervisor closure approval; do not close idea 168 from this packet alone.

Completed route summary:
- Parser compatibility overloads for typedef lookup, const-int rendering, and
  record-tag/rendered-record lookup were retired or fenced behind explicit
  no-metadata compatibility boundaries.
- Sema/consteval rendered enum compatibility, consteval type-binding mirrors,
  and NTTP binding mirrors were fenced so complete structured carriers remain
  authoritative.
- HIR rendered/template bridges were narrowed across specialization lookup,
  struct layout, declaration lookup, template-def preservation lookup, static
  member rendered maps, member symbol rendered lookup, template global rendered
  lookup, struct method rendered maps, and ref-overload rendered maps.
- LIR/HIR-to-LIR bridges were narrowed for extern decl raw maps, initializer
  reachability text scans, `type_decls` legacy shadowing, and `signature_text`
  fallback.
- BIR/backend bridges were narrowed for initializer raw symbol parsing and raw
  function/global symbol compatibility maps.
- The Step 4 review artifact
  `review/168_step4_hir_bridge_route_review.md` was already accounted for and
  remains transient/unmodified.

Retained compatibility bridges:

| Owner | Domain | Caller class | Limitation | Removal condition |
| --- | --- | --- | --- | --- |
| Parser | typedef / record / const-int no-metadata compatibility | Legacy or raw-input callers that lack structured `TextId`/record metadata | Explicit compatibility/no-metadata entry only; complete structured metadata misses must fail closed | Remove when all surviving callers can carry structured ids or record definitions |
| Sema / consteval | rendered enum and type/NTTP binding mirrors | Compatibility for incomplete imported or legacy carriers | Structured scoped enum, canonical symbol, type binding, and NTTP binding data remain authority when present | Remove after import/consteval carriers preserve the full structured binding across all call paths |
| HIR | rendered declaration, specialization, static/member/template maps | Import, no-owner handoff, preservation lookup, diagnostic/dump, or incomplete-owner compatibility | Not ordinary production authority after complete owner/index/binding metadata exists; bridge names/comments identify compatibility role | Remove after module imports and template/member preservation paths carry concrete ids, owner indexes, or structured binding keys end-to-end |
| LIR / HIR-to-LIR | extern raw map, legacy type/signature text, initializer reachability text | Final-output parity, legacy handoff, or no-id compatibility | Final rendered text may be printed or checked for parity, but typed `LinkNameId`, `StructNameId`, signature/type mirrors, and initializer ids own semantic lookup | Remove after LIR construction and reachability no longer need legacy text carriers for no-id or parity-only paths |
| BIR / backend | initializer raw symbol parser and raw function/global symbol maps | Raw-import, no-id producer, runtime/intrinsic, or verifier compatibility | Present `LinkNameId` metadata is authoritative and mismatches/misses fail closed instead of falling back to raw spelling | Remove after all BIR producers/importers preserve `LinkNameId` for user, extern, global, initializer, and prepared-link handoffs |

Scope split verified:
- Idea 169 owns route-local value, block, slot, prepared-value, prepared-label,
  generated-name, and other local identity cleanup. Those leftovers were not
  absorbed into idea 168 unless raw spelling acted as source/link-visible bridge
  authority.
- Idea 170 owns string-authority regression guard and allowlist work. Step 7
  should not add the guard machinery or turn retained-bridge vocabulary into a
  new allowlist inside this idea.

## Suggested Next

Supervisor closure decision is the next lifecycle action. Suggested final
proof policy:
- Because idea 168 changed parser, sema/consteval, HIR, LIR, BIR, and backend
  surfaces, closure-grade proof should be the repo-native full suite:
  `ctest --test-dir build -j --output-on-failure`.
- The accepted full-suite baseline at `272c82a2a` was 3135/3135 and there have
  been no code changes since that baseline, only todo-only lifecycle state. If
  the supervisor wants to avoid rerunning the full suite before the closure
  commit, that baseline plus `git diff --check -- todo.md` is defensible for
  this todo-only consolidation.
- If using a focused pre-close subset instead of the full suite, use:
  `ctest --test-dir build -j --output-on-failure -R '^(frontend_parser_tests|frontend_parser_lookup_authority_tests|frontend_hir_lookup_tests|cpp_hir_parser_.*structured_metadata|cpp_hir_sema_.*structured_metadata|cpp_positive_sema_lookup_value_structured_metadata|cpp_hir_template_.*structured_metadata|frontend_lir_.*|backend_lir_to_bir_notes|backend_cli_dump_bir_is_semantic|backend_cli_dump_prepared_bir_is_prepared|backend_prepare_structured_context)$'`
- If closure moves `plan.md`, `todo.md`, or the source idea, rerun
  `git diff --check` on the changed lifecycle files before committing.

## Watchouts

Closure readiness: ready for supervisor review, not closed.

Exact blockers: none recorded for idea 168's bridge-retirement scope.

Remaining risks:
- Retained compatibility APIs still exist by design. Review should reject only
  broad ordinary production fallback after complete structured metadata misses,
  not display/diagnostic/raw-import/no-id compatibility that is explicitly
  fenced.
- Full-suite freshness depends on the supervisor's choice: either accept the
  code-identical `272c82a2a` 3135/3135 baseline or rerun the full suite as the
  final closure proof.
- Route-local cleanup pressure remains in idea 169; guard/allowlist pressure
  remains in idea 170.

## Proof

Historical accepted full-suite baseline: 3135/3135 at `272c82a2a`.

Recommended closure-grade proof:
`ctest --test-dir build -j --output-on-failure`

Todo-only validation for this packet:
`git diff --check -- todo.md`
