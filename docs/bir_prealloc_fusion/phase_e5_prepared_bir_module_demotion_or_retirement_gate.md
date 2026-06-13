# Phase E5 PreparedBirModule Demotion Or Retirement Gate

Status: Step 3 gate artifact drafted.

Source idea:
`ideas/open/239_phase_e5_prepared_bir_module_demotion_or_retirement_gate.md`

## Scope

This document is the durable Phase E5 readiness gate for deciding whether the
current E0 through E4 evidence is sufficient to demote or retire selected
`PreparedBirModule` or `PreparedFunctionLookups` field groups, rewrite or open
draft 155, or create final follow-up implementation ideas.

The answer is conservative. The prerequisite analysis proves selected helper,
private-pass-context, diagnostic/oracle, and x86 wrapper-adjacent boundaries.
It does not prove whole `PreparedBirModule` retirement, whole
`PreparedFunctionLookups` retirement, broad prepared aggregate replacement,
broad x86 wrapper migration, riscv readiness, or cross-target wrapper
convergence.

E5 therefore keeps prepared aggregate and lookup delivery retained while
naming only narrow future adapter routes that could be drafted later.

## Prerequisite Disposition

| Prerequisite | E5 status | Gate conclusion |
| --- | --- | --- |
| E0 field-by-field ownership map | Closed and available in `docs/bir_prealloc_fusion/phase_e0_prepared_bir_merge_readiness.md`. | Usable as the ownership baseline: BIR owns target-neutral semantics, prepared owns target and policy products, and fallback/oracle/string authority stays prepared-visible. |
| E1 semantic duplicate migrations | Closed for selected helper families and recorded in `docs/bir_prealloc_fusion/phase_e1_semantic_duplicate_candidate_triage.md`. | Usable as selected-helper evidence only. It does not make aggregate BIR semantic forwarding or prepared aggregate deletion ready. |
| E2 public lookup/API contractions | Closed for selected private-pass-context helper boundaries and recorded in `docs/bir_prealloc_fusion/phase_e2_prepared_lookup_api_private_pass_context_readiness.md`. | Usable as private-reader evidence only. It does not make any `PreparedFunctionLookups` group deletion-, privatization-, or aggregate-replacement-ready. |
| E3 diagnostic/oracle replacements | Closed for row-scoped candidates and recorded in `docs/bir_prealloc_fusion/phase_e3_prepared_diagnostic_oracle_replacement_readiness.md`. | Usable as row-specific diagnostic/oracle evidence only. It does not retire broad printer/debug, route-debug, helper-oracle, expected-string, or baseline authority. |
| E4 cross-target wrapper readiness | Closed and recorded in `docs/bir_prealloc_fusion/phase_e4_cross_target_wrapper_convergence_readiness.md`. | Usable for exactly one x86 Route 6 scalar `i32` route-debug / `ConsumedPlans` follow-up boundary. It left riscv, broad x86, and cross-target wrapper migration blocked. |
| Idea 238 | Closed under `ideas/closed/238_phase_e4_x86_route6_scalar_i32_route_debug_consumedplans_compatibility_follow_up.md`. | Prerequisite-complete only for the x86 Route 6 scalar `i32` route-debug / `ConsumedPlans` compatibility boundary. It is not broad x86 call-wrapper, riscv, route-wide wrapper, cross-target, or aggregate retirement evidence. |
| Baseline and string authority | Retained across E0 through E4 as non-regression guardrails. | Expected strings, helper-oracle names/status labels, supported-path status, wrapper output, prepared printer/debug output, route-debug output, fallback behavior, and baselines remain authoritative. |

No required prerequisite document is missing for the Step 3 gate. The available
evidence is sufficient to keep blockers explicit and to shape later narrow
follow-up ideas, but not sufficient to open broad retirement work.

## Draft 155 Disposition

Draft 155 should remain blocked as a broad prepared aggregate retirement plan.
E5 should not open it in its current broad form.

A later lifecycle packet may rewrite or supersede draft 155 only if it turns
the retirement concept into one-field-group or one-adapter implementation
ideas with retained prepared fallback, diagnostic/oracle authority, wrapper
compatibility, and proof matrices. That rewrite must not claim whole
`PreparedBirModule` or whole `PreparedFunctionLookups` retirement readiness.

The safe disposition is:

- do not open draft 155 as-is;
- do not treat E0 through E4 as aggregate deletion proof;
- keep draft 155 blocked unless rewritten into narrow successor ideas;
- prefer new final follow-up ideas only for field groups whose adapter and
  proof boundaries are explicitly named by this gate.

## Field Groups Ready For Demotion Or Retirement

No `PreparedBirModule` field group is ready for whole-field deletion,
whole-field privatization, or aggregate retirement.

No `PreparedFunctionLookups` group is ready for whole-group deletion,
whole-group privatization, or aggregate replacement.

The only evidence that can support later implementation ideas is
sub-field/sub-reader evidence:

| Narrow candidate | Current readiness | Required shape before any later work |
| --- | --- | --- |
| Selected direct BIR or route semantic identity readers over `module`, selected `names`, selected `control_flow`, and selected route facts | Possible one-consumer demotion candidate. | Name one reader or helper, prove BIR/route and prepared agreement, keep aggregate prepared delivery and fallback/oracle surfaces visible. |
| Route 3 memory/source identity reads inside `memory_accesses` | Possible one-reader adapter candidate. | Prove one memory/source identity read while excluding address formation, materialization, relocation, final operands, value homes, wrappers, fallback, and diagnostics from the route-owned fact. |
| Route 4/5 publication or edge/join identity reads inside `edge_publications` and `edge_publication_source_producers` | Possible one semantic adapter candidate. | Prove one publication, edge, or join-source identity while retaining publication construction, move/home/storage policy, block order, wrappers, printer/debug, and fallback/oracle behavior. |
| Route 6 scalar call-use source identity | Possible one-reader or one-row adapter candidate. | Keep the proof no broader than the selected call argument/result role. Idea 238 proves only x86 Route 6 scalar `i32` route-debug / `ConsumedPlans` compatibility, not call-plan group deletion. |
| Route 1/2/5/6/7 source-producer diagnostic rows | Possible one row or tightly scoped row-family candidate. | Preserve prepared fallback for absent, invalid, duplicate/conflict, mismatch, policy-sensitive, wrapper, printer/debug, helper-oracle, and expected-string paths. |
| Identity-only liveness, move-bundle, value-home, or intrinsic metadata questions | Not ready; analysis candidate only. | First isolate one reader that does not consume allocation, move scheduling, storage/home, carrier/helper, target policy, or output policy. |

These candidates are not deletion permission. They are the maximum safe scope
for later idea creation.

## Field Groups That Must Remain Target Or Prepared Policy

The following `PreparedBirModule` surfaces remain target/prepared policy or
private preparation context:

- `target_profile`, `register_group_overrides`, `regalloc`, target-policy
  portions of `value_locations`, and all register/home/storage rendering
  decisions.
- `stack_layout`, `frame_plan`, `dynamic_stack_plan`, and `storage_plans`.
- `addressing` and address-materialization policy.
- target-policy portions of `call_plans`, including ABI placement, wrapper
  kind, clobbers, result lanes, outgoing stack layout, helper/carrier
  protocols, call records, storage, movement, and emitted behavior.
- `variadic_entry_plans`, `i128_carriers`, `f128_carriers`,
  `atomic_operations`, `intrinsic_carriers`, `inline_asm_carriers`,
  `f128_runtime_helpers`, and `i128_runtime_helpers`.
- pass-context and sequencing metadata in `route`, `completed_phases`, and
  policy-sensitive portions of `liveness`.

The following `PreparedFunctionLookups` groups remain retained as target or
prepared policy delivery:

- `address_materializations`;
- `move_bundles`;
- `value_homes`;
- policy-bearing portions of `call_plans`, `memory_accesses`,
  `edge_publications`, and `edge_publication_source_producers`;
- aggregate construction through `make_prepared_function_lookups(...)` and
  `make_prepared_move_bundle_lookups(...)`.

No later idea should move ABI, frame, register, stack, addressing, move,
call/helper, carrier, intrinsic, inline-asm, runtime-helper, branch spelling,
formatting, instruction-selection, emission, or wrapper-output policy into
target-neutral BIR or route ownership.

## Public Fallback, Oracle, Diagnostic, And String Surfaces To Retain

The following surfaces must remain public or prepared-visible until a later
single-boundary idea proves an equivalent owner and fallback matrix:

- `PreparedBirModule` aggregate delivery to prepared printers, debug dumps,
  helper-oracle tests, target wrappers, pass contexts, and compatibility paths.
- `PreparedFunctionLookups` aggregate delivery and public lookup groups used
  by AArch64 lowering, x86 `ConsumedPlans`, riscv prepared emission, prepared
  lookup helper tests, and wrapper/debug tests.
- Prepared fallback for absent, invalid, duplicate/conflict, ambiguous,
  mismatch, unsupported, prepared-only, policy-sensitive, and non-agreement
  cases.
- Prepared printer/debug output, route-debug output, helper-oracle
  names/status labels/strings, CLI dump snippets, supported-path contracts,
  wrapper output, expected strings, and baselines.
- `ConsumedPlans`, prepared call plans, prepared edge-publication lookups,
  selected-value metadata, direct-call/helper-oracle families, and prepared
  call/debug fallback.

These are retained authority surfaces, not merely temporary implementation
details. A later adapter can consume one proven semantic fact only while
failing closed to the existing prepared or compatibility surface.

## Compatibility Adapters Required During Any Transition

Any future demotion, privatization, or helper contraction must provide the
matching adapter before touching public prepared behavior:

| Adapter family | Required behavior |
| --- | --- |
| BIR identity adapter | Reads one function/block/value/link/CFG fact directly from BIR, verifies prepared agreement, and preserves prepared fallback and diagnostic rows. |
| Route semantic adapter | Reads one route fact for one route family and one consumer role, verifies prepared agreement, and rejects absent, invalid, duplicate/conflict, mismatch, and policy-sensitive cases through prepared fallback. |
| Private pass-context adapter | Moves only a selected agreement-gated read behind private pass context while leaving public prepared fallback/oracle APIs available for tests, diagnostics, wrappers, and compatibility consumers. |
| Diagnostic/oracle row adapter | Explains exactly one positive row from BIR/route evidence and keeps prepared authority for every non-agreement, wrapper, helper-oracle, expected-string, and baseline-sensitive path. |
| Target-policy adapter | Owns a future explicit target-policy product only if a separate idea proves every consumer, diagnostic, fallback, and cross-target requirement. No current field group has that proof. |
| Wrapper compatibility adapter | Consumes one already-proven semantic fact while preserving x86/riscv/AArch64 ABI, formatting, emission, route-debug, wrapper output, `ConsumedPlans`, prepared fallback, and expected strings. Idea 238 is the only closed example and is limited to x86 Route 6 scalar `i32` route-debug / `ConsumedPlans`. |

Adapters must be agreement-gated and fail closed. Facade renames, aggregate
reshuffles, public API hiding, or new wrappers that preserve old failure modes
do not count as readiness.

## Final Proof Strategy

Each later final packet must define proof at the same boundary it changes.
The minimum proof matrix is:

- positive agreement for the exact reader, helper, row, or wrapper boundary;
- absent, invalid, duplicate/conflict, ambiguous, mismatch, unsupported,
  prepared-only, policy-sensitive, and non-agreement fallback where relevant;
- unchanged public prepared fallback and compatibility behavior;
- unchanged printer/debug, route-debug, helper-oracle names/status labels,
  wrapper output, supported-path status, expected strings, and baselines;
- nearby same-feature coverage that proves the change is semantic and not
  fixture-shaped;
- target-output no-change proof whenever ABI, addressing, frame, value-home,
  move, call, branch, formatting, instruction-selection, emission, or wrapper
  surfaces are adjacent;
- cross-target proof before any x86-only evidence is generalized to riscv or
  to a shared wrapper family;
- full-suite or supervisor-selected regression-guard evidence before closure
  of any implementation idea that changes behavior or public surface shape.

Baseline refreshes, expectation rewrites, helper renames, unsupported
downgrades, timeout masking, wrapper-output relabeling, and named-case
shortcuts are reject signals, not proof.

For this Step 3 document packet itself, no build or CTest proof is required
because only documentation and lifecycle state are changed.

## Gate Conclusion

E5 does not authorize whole `PreparedBirModule` retirement.

E5 does not authorize whole `PreparedFunctionLookups` retirement.

E5 does not authorize opening draft 155 as a broad aggregate-retirement plan.
Draft 155 should remain blocked unless a later lifecycle packet rewrites or
supersedes it with narrow, field-group-specific successor ideas.

The only safe follow-up direction is narrow idea creation for one proven
semantic reader, one public/private adapter, one diagnostic/oracle row, or one
wrapper compatibility boundary at a time. Each such idea must retain target
policy, prepared fallback, diagnostics/oracles, public compatibility, wrapper
output, expected strings, and baseline authority until its own proof matrix is
green.
