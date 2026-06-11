Status: Active
Source Idea Path: ideas/open/198_phase_d_phase_e_lifecycle_naming_cleanup.md
Source Plan Path: plan.md
Current Step ID: Step 1
Current Step Title: Inventory Phase E-like Closed Ideas and Existing References

# Current Packet

## Just Finished

Step 1 inventory is complete for idea 198, `Phase D versus Phase E lifecycle
naming cleanup`.

Closed idea inventory:

- 182 has a Phase E-like filename, but its goal and closure were one Route 4
  AArch64 dispatch-publication reader migration:
  `current_block_entry_publication_register(...)`, with prepared publication
  fallback/oracles retained and no prepared publication API contraction.
- 183 has a Phase E-like filename, but its goal and closure were one Route 5
  AArch64 current-block join-source reader migration behind
  `build_current_block_join_prepared_query_routing(...)`, with prepared
  current-block join-source and edge-publication surfaces retained.
- 184 has a Phase E-like filename, but its goal and closure were one Route 1
  publication-source consumer migration:
  `value_publication_may_read_register_index(...)`, with prepared producer,
  constant, scalar operand, value-publication, home, storage, move, and
  machine-record policy retained.
- 185 has a Phase E-like filename, but its goal and closure were one Route 2
  AArch64 scalar ALU control-publication `select.result` path migration in
  `lower_scalar_select_publication(...)`, with prepared select-chain and
  direct-global helpers retained.
- 186 has a Phase E-like filename, but its goal and closure were one Route 3
  AArch64 indirect-callee stored-value source migration, with prepared memory
  fallback and target-addressing policy retained.
- 187 has a Phase E-like filename, but its goal and closure were one Route 6
  direct-global select-chain call-argument materialization migration, with
  call ABI, helper, carrier, and call-record policy retained.
- 188 has a Phase E-like filename, but its goal and closure were one Route 7
  AArch64 materialized-compare condition branch migration:
  `aarch64::codegen::lower_materialized_compare_condition_branch(...)`, with
  prepared and emitted-condition fallback retained.
- 189 has a Phase E-like filename, but its goal and closure were one
  cross-target interface reuse point: x86 `ConsumedPlans` reused the
  AArch64-proven Route 6 call-use source view for one scalar argument-source
  boundary, with x86 ABI, frame, register, wrapper, and emission policy
  retained.

Current references that could create citation hazards:

- The closed filenames and titles for ideas 182-189 all use `Phase E` or
  `phase_e_*`, while their bodies say they are bounded Phase D follow-up
  implementation slices sourced from
  `docs/bir_prealloc_fusion/phase_d_mir_consumer_switch_plan.md`.
- `docs/bir_prealloc_fusion/README.md` still lists
  `phase_e_prepared_bir_module_retirement.md` as the future idea-155 artifact.
  That is correct for true Phase E, but short references to "Phase E" near
  ideas 182-189 could be confused with that future retirement artifact.
- `docs/bir_prealloc_fusion/residual_route_view_consumer_migration_map.md`
  cites ideas 182-188 as selected-consumer migrations and idea 189 as one x86
  Route 6 interface reuse point. This is mostly safe, but without the readiness
  audit language a reader could over-weight the historical `phase_e_*`
  filenames.
- `docs/bir_prealloc_fusion/cross_target_route_view_reuse_map.md` names the
  x86 Route 6 scalar helper as the only ready cross-target reuse point and
  rejects broader x86/riscv generalization. It resolves the interface-reuse
  part of the ambiguity but does not itself classify the old filenames.
- `docs/bir_prealloc_fusion/return_chain_import_and_naming.md`,
  `docs/bir_prealloc_fusion/prepared_function_lookups_ownership_readiness_map.md`,
  and `docs/bir_prealloc_fusion/prepared_diagnostics_oracle_replacement_plan.md`
  repeatedly reject broad `PreparedBirModule` retirement and draft 155
  readiness, but they do not spell out the 182-189 filename hazard.

Current references that already resolve the ambiguity:

- `docs/bir_prealloc_fusion/phase_d_followup_pre_phase_e_readiness.md` states
  directly that ideas 182-189 are Phase D follow-up implementation slices with
  Phase E-like filenames, not the true Phase E `PreparedBirModule` retirement
  plan.
- That readiness audit has a route-coverage table naming each selected
  consumer or x86 interface boundary and the unproven residual scope.
- Its retirement-blocker and draft-155-readiness sections state that draft 155
  remains draft-only until field ownership, fallback/oracle consumers,
  diagnostics, cross-target breadth, return-chain import, and Route 3 boundary
  prerequisites are handled.
- Idea 198 itself records the durable naming hazard and the rule to preserve
  historical filenames instead of treating label cleanup as retirement
  progress.

## Suggested Next

Execute Step 2: classify each of ideas 182-189 as Phase D follow-up evidence,
name the selected consumer or interface boundary it proved, and list the
prerequisite maps that future draft 155 work must cite before any true Phase E
`PreparedBirModule` retirement analysis is activated or rewritten.

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

## Proof

Docs/lifecycle-only inventory packet. Inspected the active plan, source idea
198, closed ideas 182-189, draft 155, and current docs/open-idea references
with targeted `sed` and `rg` searches. No build was required and no
`test_after.log` was generated.
