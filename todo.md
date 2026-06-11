Status: Active
Source Idea Path: ideas/open/198_phase_d_phase_e_lifecycle_naming_cleanup.md
Source Plan Path: plan.md
Current Step ID: Step 2
Current Step Title: Classify Lifecycle Meaning and Draft 155 Prerequisites

# Current Packet

## Just Finished

Step 2 classification is complete for idea 198, `Phase D versus Phase E
lifecycle naming cleanup`.

Lifecycle meaning for ideas 182-189:

- 182 is Phase D follow-up evidence for the Route 4 AArch64
  dispatch-publication reader `current_block_entry_publication_register(...)`.
  It proves one current/block-entry publication identity boundary and retains
  prepared publication fallback, edge-publication surfaces, x86 wrappers,
  printer/debug, and oracle consumers.
- 183 is Phase D follow-up evidence for the Route 5 AArch64 current-block
  join-source reader behind
  `build_current_block_join_prepared_query_routing(...)`. It proves one indexed
  current-block join-source boundary and retains edge-publication,
  source-producer, move-bundle, printer/debug, and oracle surfaces.
- 184 is Phase D follow-up evidence for the Route 1 publication-source
  consumer `value_publication_may_read_register_index(...)`. It proves one
  same-block source-producer/publication-producer boundary and retains producer,
  constant, scalar operand, value-publication, home, storage, move, and final
  machine-record policy.
- 185 is Phase D follow-up evidence for the Route 2 AArch64 scalar ALU
  control-publication `select.result` path in
  `lower_scalar_select_publication(...)`. It proves one select-root /
  direct-global dependency boundary and retains prepared select-chain,
  direct-global, materialization, publication, call, memory, printer, and
  oracle consumers.
- 186 is Phase D follow-up evidence for the Route 3 AArch64 indirect-callee
  stored-value source consumer. It proves one stored-source / memory semantic
  source boundary and retains prepared memory fallback plus target-addressing,
  address-materialization, memory-operand, and oracle policy.
- 187 is Phase D follow-up evidence for the Route 6 direct-global select-chain
  call-argument materialization consumer. It proves one call-use source
  identity boundary and retains call ABI, argument/result plans, helper/carrier
  protocols, value-home/move/publication policy, and call-record spelling.
- 188 is Phase D follow-up evidence for the Route 7 AArch64 materialized
  compare condition branch consumer
  `aarch64::codegen::lower_materialized_compare_condition_branch(...)`. It
  proves one materialized-condition provenance boundary and retains comparison
  fallback, fused legality, condition-code and branch spelling policy, ALU
  policy, printer/debug, and oracle consumers.
- 189 is Phase D follow-up evidence for one cross-target interface reuse point:
  x86 `ConsumedPlans` reuses the AArch64-proven Route 6 call-use source view at
  the scalar `i32` argument-source boundary. It retains x86 ABI, frame,
  register, wrapper, move, formatting, and emission policy, and it proves no
  riscv reuse.

Prerequisite maps now available for future draft 155 work:

- `docs/bir_prealloc_fusion/phase_d_followup_pre_phase_e_readiness.md`
  supplies the top-level readiness audit: ideas 182-189 are useful Phase D
  follow-up evidence with Phase E-like filenames, but they are not the true
  Phase E retirement plan.
- `docs/bir_prealloc_fusion/residual_route_view_consumer_migration_map.md`
  supplies the route-family residual consumer map and one-consumer-at-a-time
  future implementation boundaries for Routes 1-7.
- `docs/bir_prealloc_fusion/cross_target_route_view_reuse_map.md` supplies the
  cross-target boundary map: only the x86 Route 6 scalar call-use source helper
  is ready; other x86 boundaries are blocked/unknown/target-local, and riscv
  has no ready route-view reuse point.
- `docs/bir_prealloc_fusion/prepared_function_lookups_ownership_readiness_map.md`
  supplies the field-group ownership map for `PreparedFunctionLookups`; it
  classifies groups as retained policy, compatibility/pass context, partial
  semantic route candidates, or adapter-only surfaces, with no deletion-ready
  aggregate group.
- `docs/bir_prealloc_fusion/prepared_diagnostics_oracle_replacement_plan.md`
  supplies the diagnostic and oracle replacement checklist required before
  prepared printer, dump, route-debug, wrapper, or helper-oracle surfaces can
  contract.
- `docs/bir_prealloc_fusion/return_chain_import_and_naming.md` supplies the
  durable Route 8 return-chain owner/schema import note. It authorizes citing
  return-chain identity as a separate completed route line and rejects folding
  it into Routes 1 or 7, predecessor rescans, name matching, or broad prepared
  retirement.
- `docs/bir_prealloc_fusion/phase_c_private_cache_contraction.md` and
  `docs/bir_prealloc_fusion/phase_d_mir_consumer_switch_plan.md` remain
  directly relevant background: they explain why selected migrations are
  bounded and why `PreparedFunctionLookups`/`PreparedBirModule` are not BIR
  annotation schemas or whole-route contraction points.

What these prerequisites authorize for draft 155:

- Future draft 155 activation or rewrite can cite a stable prerequisite set:
  selected route-view evidence, residual consumer inventory, cross-target
  wrapper readiness, `PreparedFunctionLookups` lookup-group ownership,
  diagnostics/oracle blockers, and Route 8 return-chain naming.
- Future draft 155 planning can ask field-by-field `PreparedBirModule`
  questions with better inputs: which fields are durable semantic state, which
  are BIR annotations, which remain private pass context, which are
  target/prepared policy, which require adapters, and which consumers still
  block contraction.
- Future implementation ideas can be split by one route family, one lookup
  group, one diagnostic/oracle surface, or one target-wrapper boundary, while
  retaining prepared fallback and target-policy exclusions.

What these prerequisites do not authorize:

- They do not open or execute draft 155 under this naming-cleanup plan.
- They do not prove `PreparedBirModule` retirement, demotion, deletion,
  field-level contraction, or replacement by a BIR-owned aggregate.
- They do not prove route-wide completion for any Route 1-7 family from the
  selected-consumer migrations in ideas 182-188.
- They do not prove broad `PreparedFunctionLookups` contraction, deletion, or
  privatization.
- They do not authorize weakening prepared printer, dump, route-debug, wrapper,
  helper-oracle, c_testsuite, unsupported-marker, or baseline expectations.
- They do not move ABI, address, frame, storage, register, move, publication,
  branch, comparison, formatting, or final-emission policy into BIR route
  ownership.
- They do not prove broad x86 reuse, any riscv reuse, or return-chain evidence
  outside the distinct Route 8 owner/schema line.

## Suggested Next

Execute Step 3: define stable citation language for ideas 182-189. Suggested
positive wording: "Phase D follow-up selected-consumer migration with a
Phase E-like filename." Suggested negative wording: do not cite ideas 182-189
as true Phase E retirement, `PreparedBirModule` contraction, draft 155
completion, route-wide migration, prepared API deletion, or cross-target/riscv
readiness.

## Watchouts

- Preserve historical closed idea filenames and contents.
- Do not open, rewrite, or execute draft 155 under this naming-cleanup plan.
- Do not edit implementation, tests, expectations, unsupported markers, or
  prepared API contracts.
- Do not claim `PreparedBirModule` retirement progress from selected-consumer
  migrations or label cleanup.
- Treat bare `Phase E` references to ideas 182-189 as citation hazards unless
  they also say "Phase D follow-up" or "Phase E-like filename".
- Do not use idea 189 as evidence for broad x86 or riscv migration readiness;
  it proves only one x86 Route 6 scalar argument-source interface boundary.
- Do not treat the prerequisite maps as permission to remove prepared fallback,
  diagnostics, target-wrapper surfaces, or oracle tests; they mostly classify
  blockers and split future work.

## Proof

Docs/lifecycle-only classification packet. Inspected the active plan, source
idea 198, current `todo.md`, readiness audit, residual consumer migration map,
cross-target reuse map, `PreparedFunctionLookups` ownership map,
diagnostics/oracle replacement plan, return-chain import note, draft 155, and
closed ideas 182-189 with targeted `sed` and `rg` searches. No build was
required and no `test_after.log` was generated.
