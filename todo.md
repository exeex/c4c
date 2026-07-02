Status: Active
Source Idea Path: ideas/open/442_pointer_value_memory_provenance_publication.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Decide Concrete, Opaque, Or Unsupported Policy

# Current Packet

## Just Finished

Step 2 - Decide Concrete, Opaque, Or Unsupported Policy completed for the
residual `930930-1` pointer-value memory rows.

Policy decision:

- No residual `930930-1` pointer-value row is concrete today.
- A narrow `OpaqueCompatibility` policy is rejected for this route.
- The three `reg1` pointer-value loads are intentionally unsupported until a
  producer publishes formal pointer authority and concrete object/range facts.
- The `%mr_TR - 8` store remains pointer-delta-later-work and must stay
  fail-closed until base formal pointer authority is available.

Reason `OpaqueCompatibility` is rejected:

- Closed idea 438 permits an explicit opaque policy only for a producer-owned
  runtime pointer compatibility class with its own coverage. These rows are not
  inherently opaque runtime pointer accesses; they have plausible concrete
  `@mem` sources, but the missing fact is authority to attach those sources to
  external-linkage formals.
- Accepting `layout_authority=unknown` plus `range_verdict=unknown_compatible`
  here would bypass `prepared_pointer_value_memory_has_proven_authority` and
  move provenance inference into the RV64 target.
- The only available positive signals are same-module direct calls and
  computed-address call-argument dumps for external-linkage `930930-1::f`.
  Ideas 443, 444, and 445 explicitly rejected those signals as complete
  authority: `NoExternalCaller` remains unpopulated and
  `FormalPointerAuthorityKind` for `%p.reg1`/`%p.mr_TR` remains `Unknown`.
- A current opaque acceptance would be testcase-shaped around `930930-1`,
  `reg1`, or `mr_TR`, not a semantic lowering rule.

Residual row dispositions:

| Row | Disposition | Owner before support | Diagnostic precision needed |
| --- | --- | --- | --- |
| `reg1` load A, `/tmp/930930-1.step1.prepared.out:568`, `f:block_4:inst1`, `base=pointer_value`, `%t5` loaded from `%lv.param.reg1` | Unsupported. Do not publish as concrete or opaque. | Producer/formal pointer authority, not RV64 target lowering. A future producer must establish closed-world/no-external-caller authority or equivalent for external-linkage `f`, then publish concrete `@mem` provenance, object extent 800 bytes, requested range, and `ProvenInBounds` for `%p.reg1`-derived pointer values. | Current top-level `unsupported_instruction_fragment` remains an acceptable fail-closed result for this policy step. A later diagnostic-only slice may refine it to an owner-specific prepared pointer-value memory authority diagnostic naming missing `FormalPointerAuthorityKind::NoExternalCaller` or equivalent plus `layout_authority=unknown` / `range_verdict=unknown_compatible`. |
| `reg1` load B, `/tmp/930930-1.step1.prepared.out:570`, `f:logic.rhs.11:inst1`, `base=pointer_value`, `%t15` loaded from `%lv.param.reg1` | Unsupported. Same policy as load A. | Same producer/formal pointer authority owner. | Same diagnostic precision as load A; no target-side inference from same-module callsite. |
| `reg1` load C, `/tmp/930930-1.step1.prepared.out:572`, `f:block_5:inst1`, `base=pointer_value`, `%t24` loaded from `%lv.param.reg1` | Unsupported. Same policy as load A. | Same producer/formal pointer authority owner. | Same diagnostic precision as load A; no target-side inference from same-module callsite. |
| `%mr_TR - 8` pointer-delta store, `/tmp/930930-1.step1.prepared.out:575`, `f:block_5:inst5`, `base=pointer_value`, `pointer=%t27` after `%lv.param.mr_TR` load and decrement | Deferred pointer-delta-later-work. Not concrete, not opaque-compatible, and not a target-consumable row today. | First producer/formal pointer authority for `%p.mr_TR`; after that, a separate pointer-delta provenance producer must carry the constant decrement from the authorized base to prove the resulting `@mem+792` range. | If surfaced, diagnostic should distinguish missing base formal pointer authority from missing pointer-delta propagation; both are producer/prepared-fact issues, not RV64 layout inference opportunities. |
| Frame-slot carrier rows, `/tmp/930930-1.step1.prepared.out:562-567`, `:569`, `:571`, `:573-574`, `:576-579` | Unrelated to target pointer-value memory consumption. | None for idea 442 Step 2. | No new diagnostic policy. |
| Global `@mem+792` store, `/tmp/930930-1.step1.prepared.out:581`, `base=global_symbol` | Already concrete as a global-symbol row; unrelated to formal pointer provenance. | None for idea 442 Step 2. | Do not use this global layout authority to authorize `%p.reg1` or `%p.mr_TR`. |

Follow-up source ideas required before target consumption:

1. Define and implement a real `NoExternalCaller` or equivalent closed-world
   metadata source for non-internal definitions. The boundary must include a
   complete caller-set source, function-address escape coverage, indirect-call
   target exclusion, visibility/linker/LTO mode constraints, and false-by-
   default behavior. Same-module direct calls alone remain rejected.
2. After base formal authority exists, publish formal/local/load pointer
   provenance for external-linkage formals so `reg1` rows can carry concrete
   object identity, object extent, base-plus-offset, requested access range,
   and `ProvenInBounds`.
3. Only after `%p.mr_TR` has base authority, add a separate pointer-delta
   propagation idea for constant pointer add/sub chains such as `%mr_TR - 8`.
4. RV64 target consumption should remain limited to rows that already satisfy
   `prepared_pointer_value_memory_has_proven_authority` or a future named
   opaque policy with producer-owned semantics and coverage.

## Suggested Next

Delegate the next packet as a fail-closed diagnostic or handoff step for idea
442. If implementation is requested, keep it diagnostic-only unless a separate
producer-owned source idea first publishes formal pointer authority; do not add
RV64 target lowering for the residual `930930-1` rows from the current facts.

## Watchouts

Keep pointer-delta propagation deferred until base formal pointer authority is
soundly published. Do not infer provenance from same-module calls, computed
call-argument addresses, K&R formals, `@mem` global layout, object names, or
the current `UnknownCompatible` range verdict.

## Proof

`git diff --check -- todo.md`
