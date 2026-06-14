# 264 Phase F4 post-F3 prepared boundary reassessment gate

## Goal

Reassess the BIR/prepared boundary after the completed Phase F3 blocker maps,
agreement bridges, structural one-reader candidates, and baseline repair, then
produce the next bounded follow-up ideas.

This is analysis-only. It must decide whether any prepared field group,
subfield, public lookup group, compatibility surface, or private metadata field
is now ready for a narrow demotion/exit packet. It must not directly implement
that packet, delete prepared aggregates, open draft 155, or claim broad
`PreparedBirModule` retirement.

## Why This Exists

The previous gates repeatedly found that broad prepared retirement was unsafe,
but Phase F3 changed the evidence:

- Route 6 call identity parity was mapped and the x86 scalar call argument
  source identity adapter was confirmed.
- Route 3 memory/source parity was mapped, x86 `LoadLocal` agreement bridge
  support landed, and the needed fixture/compare-join support closed.
- Route 4/5 edge-publication parity was mapped and x86 Route 5/prepared edge
  publication agreement bridge landed.
- Prepared compatibility fail-closed proof requirements were documented.
- `PreparedBirModule::route` was demoted behind private storage while
  `invariants`, `completed_phases`, and `notes` stayed retained.
- `PreparedBirModule::liveness` remained blocked planning authority.
- The structural one-reader queue for `module`, `names`, `control_flow`, and
  `store_source_publications` completed all named candidates.
- The full-suite baseline timeout candidate from idea 263 was diagnosed as
  non-reproducible in isolation, the fresh full-suite proof restored
  3428/3428, and the accepted baseline is green again.

That means the next question is no longer just "what is blocked?" The question
is whether any part of prepared is now thin enough to move from public
prepared authority into one of these stable categories:

- BIR-owned semantic fact;
- target-owned policy product;
- private pass context;
- retained explicit compatibility adapter;
- deletion/demotion candidate;
- still-blocked public prepared authority.

## Required Inputs

Read the closure notes for:

- `ideas/closed/248_phase_f2_x86_riscv_prepared_boundary_completion_criteria_audit.md`
- `ideas/closed/249_phase_f3_route6_call_identity_parity_blocker_map.md`
- `ideas/closed/250_phase_f3_route3_memory_source_parity_blocker_map.md`
- `ideas/closed/251_phase_f3_route45_edge_publication_parity_blocker_map.md`
- `ideas/closed/252_phase_f3_edge_publication_source_producer_blocker_map.md`
- `ideas/closed/253_phase_f3_prepared_module_structural_exit_blocker_map.md`
- `ideas/closed/254_phase_f3_prepared_compatibility_fail_closed_proof_matrix.md`
- `ideas/closed/255_phase_f3_prepared_private_pass_context_metadata_gate.md`
- `ideas/closed/256_phase_f3_prepared_liveness_authority_blocker_map.md`
- `ideas/closed/257_phase_f3_x86_route6_call_argument_source_identity_adapter.md`
- `ideas/closed/258_phase_f3_x86_route3_loadlocal_source_memory_agreement_bridge.md`
- `ideas/closed/259_phase_f3_x86_route5_prepared_edge_publication_agreement_bridge.md`
- `ideas/closed/260_phase_f3_prepared_module_structural_one_reader_candidates.md`
- `ideas/closed/261_phase_f3_x86_route3_loadlocal_publication_fixture_support.md`
- `ideas/closed/262_phase_f3_x86_compare_join_loadlocal_selected_arm_support.md`
- `ideas/closed/263_phase_f3_full_suite_timeout_baseline_regression_diagnosis.md`

Also verify the current accepted baseline is still 3428/3428 and that no
`test_baseline.new.log` regression candidate is pending.

## In Scope

Reassess the current exit status of:

- `PreparedFunctionLookups::call_plans`;
- `PreparedFunctionLookups::memory_accesses`;
- `PreparedFunctionLookups::edge_publications`;
- `PreparedFunctionLookups::edge_publication_source_producers`;
- `PreparedBirModule::module`;
- `PreparedBirModule::names`;
- `PreparedBirModule::control_flow`;
- `PreparedBirModule::store_source_publications`;
- `PreparedBirModule::route`;
- `PreparedBirModule::invariants`;
- `PreparedBirModule::completed_phases`;
- `PreparedBirModule::notes`;
- `PreparedBirModule::liveness`;
- prepared helper/oracle/status/fallback/route-debug/printer/wrapper-output
  compatibility surfaces.

For each row, decide whether it is:

- ready for one concrete demotion/exit implementation idea;
- ready only for another analysis blocker map;
- retained compatibility authority;
- target-policy-owned and not a BIR/prepared-retirement candidate;
- private pass context already done;
- still blocked by missing x86/riscv parity or missing fail-closed proof.

## Completion Standard

The boundary is "thin enough to connect x86/riscv" for a fact family only if
all of these are true:

- a single route/BIR semantic authority is named;
- x86 consumes it through an agreement-gated path, or has explicit
  fail-closed non-applicability;
- riscv consumes it through an agreement-gated path, or has explicit
  fail-closed non-applicability;
- public prepared surfaces that remain are compatibility mirrors, not the
  semantic source of truth;
- missing, invalid, duplicate/conflict, mismatch, unsupported, prepared-only,
  route-only, fallback, and policy-sensitive rows fail closed;
- helper/oracle statuses, fallback names, route-debug output, prepared printer
  output, wrapper output, exact target output, and baseline behavior stay
  stable;
- ABI, layout, register, stack, storage, addressing, carrier/helper,
  formatting, emission, instruction spelling, and wrapper policy remain
  target-owned.

If any row fails this standard, do not mark it demotion-ready. Name the exact
missing proof or consumer boundary instead.

## Required Output

The closure note must include:

- an updated post-F3 boundary matrix for every in-scope row;
- a list of rows that are now thinner than they were after idea 247;
- a list of rows still blocked and the exact missing x86/riscv or
  compatibility proof;
- a list of rows that should remain target policy and never move to BIR;
- a list of rows that are already private pass context or ready for a private
  pass-context packet;
- follow-up idea files for each safe next packet;
- an explicit draft 155 disposition: keep blocked, rewrite, supersede, or
  retire obsolete.

Follow-up ideas must be bounded. Prefer one fact family, one lookup group,
one field sub-slice, or one private metadata field per idea. If no
implementation packet is safe, close with the blocker matrix and create only
analysis follow-ups.

## Out Of Scope

- Directly editing implementation code.
- Deleting, privatizing, or wrapping a prepared field in this gate.
- Opening draft 155 as broad retirement work.
- Claiming whole `PreparedBirModule` or whole `PreparedFunctionLookups`
  retirement from one-reader or one-target evidence.
- Moving target policy into BIR.
- Weakening expected strings, helper/oracle statuses, unsupported behavior,
  fallback behavior, wrapper output, prepared printer output, target output,
  or baseline tests.

## Reviewer Reject Signals

- The gate ignores any closure note from 248 through 263.
- The result repeats old blocker language without incorporating the completed
  F3 bridges and structural one-reader candidates.
- A row is marked demotion-ready without both x86/riscv evidence or explicit
  non-applicability, retained compatibility proof, and fail-closed behavior.
- Compatibility strings or prepared helper/oracle statuses are treated as proof
  of semantic ownership transfer.
- Public prepared authority is hidden behind a private accessor name instead
  of removed, retained explicitly, or proven as a compatibility mirror.
- The gate creates broad prepared retirement work instead of bounded next
  packets.
