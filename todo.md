Status: Active
Source Idea Path: ideas/open/168_compatibility_bridge_retirement.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Retire HIR Rendered Declaration and Template Bridges

# Current Packet

## Just Finished

Step 3 - Narrow Sema and Consteval Rendered Mirrors is complete enough to
advance. The sema/consteval bridge-retirement packets landed in:

| Commit | Result |
| --- | --- |
| `40404c298` | Fenced rendered enum-constant compatibility around `static_eval_int`; ordinary structured enum key/text misses fail closed, and the retained rendered enum map is reachable only through an explicitly named compatibility API. |
| `3eaa60230` | Fenced `ConstEvalEnv` template/type binding mirrors; complete key/text misses no longer reopen rendered lookup, while legacy rendered display-tag helpers are named as compatibility bridges. |
| `70de559c4` | Fenced `ConstEvalEnv` NTTP binding mirrors; complete NTTP key/text misses and forwarded NTTP misses fail closed, while rendered NTTP lookup remains only a no-metadata compatibility bridge. |

Step 3 completion decision:

| Field | Result |
| --- | --- |
| Sema/type-utils rendered enum maps | Covered by `40404c298`; complete scoped-carrier and TextId misses fail closed unless the caller opts into the named compatibility boundary. |
| Consteval type and NTTP binding mirrors | Covered by `3eaa60230` and `70de559c4`; key/text maps remain binding authority, rendered maps remain named legacy/no-metadata mirrors, and complete metadata misses fail closed. |
| Fallback canonical template names | Already fenced before this Step 3 slice: `substitute_template_args_impl` only consults `fallback_name_bindings` when the use-site lacks complete template parameter identity, and `cpp_hir_sema_canonical_symbol_metadata_test` covers both the complete-identity miss and no-metadata fallback cases. No extra Step 3 packet is required for idea 168. |
| Consteval local `by_name` | Not a Step 3 blocker for idea 168; the audit classifies this as route-local/generated identity cleanup owned by idea 169, not source/link compatibility bridge retirement. |
| Lifecycle decision | Advance from Step 3 to Step 4: Retire HIR Rendered Declaration and Template Bridges. |

## Suggested Next

Begin Step 4 by re-reading the idea 167 HIR bridge inventory and selecting the
first narrow HIR rendered declaration/template bridge. Candidate starting
points from the runbook are `fn_index`, `global_index`, `struct_defs`,
`template_defs`, rendered specialization keys, rendered qualified imports, and
no-owner handoffs.

The next executor packet should name the exact HIR bridge family, separate
ordinary production lookup from imports/dumps/diagnostics/incomplete-owner
compatibility, inspect production callers before editing tests, and add
closed-miss proof for the touched structured carrier.

## Watchouts

- Keep ABI, display, diagnostics, and final spelling output-only; Step 3 should
  not remove visible text just to reduce rendered-string grep count.
- Do not fold route-local generated-name cleanup into this plan; idea 169 owns
  route-local identity domains.
- HIR rendered indexes may be valid as import, dump, diagnostic,
  incomplete-owner, absent-owner-index, no-owner handoff, or display/output
  boundaries; do not remove those just to reduce grep count.
- Keep HIR `FunctionCtx` local/label/generated-name cleanup out of this plan;
  idea 169 owns route-local identity domains.
- Step 3 retained compatibility bridges must stay narrow: enum rendered maps,
  consteval type bindings, and NTTP bindings are not ordinary authority after
  complete structured metadata misses.
- The pre-existing untracked `review/166_compile_time_registry_fencing_route_review.md`
  was not touched.
- No current blockers.

## Proof

Lifecycle-only advancement. No implementation, tests, `plan.md`, source idea,
review artifact, or log files were changed by this packet.

Step 3 proof already landed with the three sema/consteval bridge commits:

- `40404c298`: built `cpp_hir_sema_consteval_type_utils_metadata_test` and
  `frontend_hir_tests`; ran 2/2 focused tests,
  `cpp_hir_sema_consteval_type_utils_structured_metadata` and
  `frontend_hir_tests`.
- `3eaa60230`: built `cpp_hir_sema_consteval_type_utils_metadata_test` and
  `frontend_hir_tests`; ran the same 2/2 focused tests.
- `70de559c4`: built `cpp_hir_sema_consteval_type_utils_metadata_test` and
  `frontend_hir_tests`; ran the same 2/2 focused tests.

Plan-owner validation for this lifecycle edit:

`git diff --check -- todo.md`
