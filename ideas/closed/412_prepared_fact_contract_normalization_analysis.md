# Prepared Fact Contract Normalization Analysis

Status: Closed
Type: Design analysis and follow-up planning idea

## Goal

Audit the current BIR/prealloc/prepared fact contract and produce a staged
improvement plan that prevents BIR/prepared producer responsibilities from
being repaired ad hoc in MIR or RV64 object emission.

This idea is an analysis and planning umbrella. It should confirm or reject the
current design concerns from evidence in code, closed RV64 gcc_torture repair
ideas, prepared dumps, and target consumers, then create focused follow-up
ideas for the concrete contract refactors.

## Why This Exists

The RV64 gcc_torture push through ideas 395 through 411 repeatedly exposed
boundaries where the target object route could only make progress safely after
producer-side facts were repaired or split into separate BIR/prepared ideas:

- `403_prepared_i16_formal_abi_publication.md`
- `404_prepared_wide_rematerialized_immediate_admission.md`
- `405_prepared_local_aggregate_slot_layout_facts.md`
- `407_prepared_i16_same_module_call_arg_abi_publication.md`
- `408_prepared_va_start_destination_address_helper_operand_publication.md`
- `409_prepared_packed_fp128_global_initializer_admission.md`
- `410_lir_to_bir_packed_bitfield_scalar_binop_semantics.md`

Those closures suggest that the core problem is not one RV64 lowering bug, but
an increasingly patched prepared fact layer between semantic BIR and target
consumers. `PreparedBirModule` now carries many parallel plans/publications,
`PreparedFunctionLookups` is a bag of target-facing lookup tables, and several
prepared records use broad optional-field structs rather than typed contracts.

The risk is that future gcc_torture work will continue adding one-off prepared
vectors, missing-fact enums, or RV64 consumer heuristics instead of designing
clear producer-owned fact contracts and verifier boundaries.

## In Scope

- Audit `src/backend/bir/`, `src/backend/prealloc/`, and target prepared
  consumers enough to classify prepared fact ownership and contract shape.
- Use closed ideas 395 through 411 as evidence for recurring contract gaps,
  not as implementation instructions.
- Confirm or reject these suspected refactor areas:
  - producer-owned ABI/home facts for subword scalars and same-module calls
  - helper/intrinsic operand home publication, especially `va_start`
  - local aggregate/union and global initializer storage/layout facts
  - rematerialized constant/materialization facts
  - call-argument source selection and other optional-field bags that should
    become typed variants
  - prepared contract verification before target consumers run
- Identify which facts belong to semantic BIR, prepared/prealloc, target
  lowering, or diagnostics/admission.
- Produce a concise design report under `review/` with:
  - current architecture map
  - evidence-backed problem list
  - proposed normalized contract boundaries
  - migration order
  - risks and test strategy
- Create follow-up ideas under `ideas/open/` for each concrete refactor or
  implementation slice that should proceed after the analysis.

## Out Of Scope

- Performing the contract refactor implementation in this idea.
- Rewriting BIR IR wholesale before proving the prepared contract boundary
  needs it.
- Adding new RV64 object-emission fixups for missing producer facts.
- Changing gcc_torture allowlists, weakening runtime comparison, or marking
  cases unsupported.
- Folding unrelated frontend/parser/HIR work into this contract analysis.
- Treating one named gcc_torture testcase as sufficient proof for a contract
  refactor.

## Required Analysis

1. Map the current fact pipeline:
   - semantic BIR production
   - prealloc/prepared publication
   - prepared lookup construction
   - MIR/RV64 prepared consumer usage
   - fail-closed diagnostics
2. For each suspected refactor area, answer:
   - what fact is missing or over-broad today
   - who should own producing it
   - who may consume it
   - what invariant should be verified before target lowering
   - what current code or closed idea proves the issue is real
3. Audit optional-field records, especially call-argument and memory/publication
   records, for variant candidates.
4. Audit existing target-side checks that look like producer-fact recovery or
   semantic reconstruction.
5. Define a staged migration plan that reduces risk:
   - verifier/reporting first where possible
   - typed contract extraction before target behavior changes
   - one producer fact family per implementation idea
   - matching focused tests plus RV64 gcc_torture representative proof
6. Generate follow-up ideas for the staged plan. Each generated idea must have
   concrete reviewer reject signals that prevent RV64/MIR fixups from masking
   producer defects.

## Acceptance Criteria

- The design report exists under `review/` and answers every required analysis
  point above.
- The report identifies which concerns are confirmed, rejected, or still
  inconclusive, with file/idea evidence.
- The report includes a prioritized migration plan.
- Follow-up ideas are created for the concrete refactor slices that should be
  implemented after this analysis.
- Any area not converted into a follow-up idea has an explicit reason, such as
  insufficient evidence, already-covered ownership, or out-of-scope layer.
- No implementation files are changed by this analysis idea.
- No active `plan.md` / `todo.md` state is required unless this idea is later
  activated through the normal lifecycle.

## Closure Notes

- 2026-06-27: Closed after producing
  `review/412_prepared_fact_contract_normalization_report.md`.
- The report confirms the main concern: the design debt is concentrated in the
  BIR/prealloc/prepared fact contract and target consumer interface, not in a
  wholesale BIR IR rewrite.
- Generated six follow-up ideas:
  - `ideas/open/413_prepared_contract_verifier_and_owner_taxonomy.md`
  - `ideas/open/418_prepared_target_consumer_boundary_audit.md`
  - `ideas/open/414_typed_prepared_call_argument_contracts.md`
  - `ideas/open/415_prepared_value_materialization_contracts.md`
  - `ideas/open/416_prepared_helper_operand_home_contracts.md`
  - `ideas/open/417_prepared_storage_layout_and_initializer_contracts.md`
- The intended migration order is `413 -> 418 -> 414/415/416/417`, with 418
  feeding target-consumer findings into the typed contract slices.
- The generated ideas explicitly preserve the user guardrail: RV64 gcc_torture
  is evidence and may temporarily regress during contract normalization, but
  default `ctest --test-dir build -j --output-on-failure` / normal CTest must
  not regress.
- No implementation files were changed by this analysis.

## Reviewer Reject Signals

- Reject analysis that only restates "prepared is messy" without pointing to
  concrete structs, consumers, diagnostics, closed ideas, or failing buckets.
- Reject follow-up ideas that allow MIR/RV64 object emission to infer ABI
  homes, aggregate layout, global initializer bytes, helper operand addresses,
  or BIR constant semantics when producer facts are missing.
- Reject a plan that proposes a broad rewrite of BIR or prepared lowering
  without a staged verifier/contract migration path.
- Reject grouping unrelated producer families into one oversized implementation
  idea when they can be sequenced independently.
- Reject testcase-shaped ideas whose main proof is one named gcc_torture case
  without nearby same-feature coverage or a semantic contract test.
- Reject diagnostics-only churn claimed as contract progress when the same
  missing producer fact can still reach RV64/MIR consumers.
