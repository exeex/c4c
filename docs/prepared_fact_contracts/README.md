# Prepared fact contract artifacts

This directory is the shared analysis workspace for the prepared fact contract
normalization program.

Ideas 413 through 418 should write durable analysis artifacts here. Closure
notes may summarize the result, but cross-idea handoff data belongs in these
files so later ideas can consume explicit taxonomy and row-level owner
decisions without mining closed idea text.

## Idea Artifacts

- `contract_taxonomy_and_fact_family_matrix.md`
  - Produced by idea 413.
  - Defines the verifier owner taxonomy, fact-family classification vocabulary,
    accepted diagnostic categories, and initial rows for value homes, call
    argument plans, memory accesses, helper operand homes, and publication
    facts.
  - This is the required input for idea 418 and for ideas 414-417.
- `target_consumer_boundary_audit.md`
  - Produced by idea 418.
  - Consumes the idea 413 taxonomy and classifies RV64/AArch64 target consumer
    findings as coherent-fact lowering, target scheduling over explicit facts,
    producer contract gaps, or semantic BIR gaps.
  - This is the required target-consumer input for ideas 414-417.
- `call_argument_contract_plan.md`
  - Produced or updated by idea 414.
  - Consumes the taxonomy and target-consumer audit rows that mention call
    argument plans, call-use sources, argument homes, byval lanes, preserved
    values, and outgoing call storage.
- `value_materialization_contract_plan.md`
  - Produced or updated by idea 415.
  - Consumes rows that mention rematerialized immediates, pointer deltas,
    same-block producer chains, select/binary producer recovery, and
    materialization scheduling authority.
- `helper_operand_home_contract_plan.md`
  - Produced or updated by idea 416.
  - Consumes rows that mention variadic helpers, intrinsic/runtime helper
    operand homes, helper access plans, and helper-specific missing-fact
    diagnostics.
- `storage_initializer_contract_plan.md`
  - Produced or updated by idea 417.
  - Consumes rows that mention local storage extents, alias/overlay authority,
    memory provenance, global initializer bytes, relocations, zero-fill, and
    unsupported object-data forms.

## Handoff Rules

- Treat `contract_taxonomy_and_fact_family_matrix.md` as the source of the
  shared vocabulary. Later ideas may extend it only when they need a new
  category that cannot be represented by the existing taxonomy.
- Treat `target_consumer_boundary_audit.md` as the row-level handoff from 418
  into 414-417. Every downstream contract idea should cite the relevant audit
  rows it consumes, or explicitly record that no row applies to its selected
  implementation slice.
- If a later idea disagrees with an earlier row, record the correction in the
  later artifact and propose a follow-up idea to reconcile the source artifact.
- Do not use these documents to bypass the single-plan lifecycle. Actual work
  still starts from `ideas/open/`.
- RV64 gcc_torture evidence may be recorded here, but default CTest stability
  remains the non-regression gate for these contract-normalization ideas.
