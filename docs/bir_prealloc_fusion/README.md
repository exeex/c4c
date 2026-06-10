# BIR/prealloc fusion artifacts

This directory is the shared analysis workspace for the BIR/prealloc fusion
program.

The phase ideas under `ideas/draft/151_*` through `ideas/draft/155_*` should
write durable analysis artifacts here. Closure notes may summarize the result,
but cross-phase handoff data belongs in these files so later phases can consume
it without mining closed idea text.

## Phase Artifacts

- `phase_a_normalization_candidates.md`
  - Produced by idea 151.
  - Classifies `Prepared*` facts that should become BIR-owned normalization
    relationships, facts rejected from BIR, dependency order, and follow-up
    idea payloads.
- `phase_b_annotation_schema_candidates.md`
  - Produced by idea 152.
  - Consumes Phase A and maps candidate BIR annotation schema locations,
    private/cache-only fields, target-facing rejects, and schema follow-ups.
- `phase_c_private_cache_contraction.md`
  - Produced by idea 153.
  - Consumes Phases A and B and maps prealloc caches/indexes that can become
    private once BIR relationships or annotations exist.
- `phase_d_mir_consumer_switch_plan.md`
  - Produced by idea 154.
  - Consumes Phases A-C and maps MIR/codegen consumers from prepared APIs to
    BIR annotated-view APIs.
- `phase_e_prepared_bir_module_retirement.md`
  - Produced by idea 155.
  - Consumes Phases A-D and maps the final retirement or demotion path for
    `PreparedBirModule`.

## Rules

- Keep phase artifacts analysis-oriented. They should produce follow-up idea
  payloads, not direct implementation patches.
- Treat earlier phase artifacts as inputs, not as stale commentary.
- If a later phase disagrees with an earlier artifact, record the correction in
  the later artifact and propose a follow-up idea to reconcile the source
  phase.
- Do not use these documents to bypass the single-plan lifecycle. Actual work
  still starts from `ideas/open/`.
