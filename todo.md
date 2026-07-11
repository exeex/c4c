Status: Active
Source Idea Path: ideas/open/691_prepared_mir_view_equivalence_dump_comparator_mvp.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Add Canonical Core View Dump

# Current Packet

## Just Finished

Activation created the runbook and aligned execution metadata for Step 1.

## Suggested Next

Implement `plan.md` Step 1: add a deterministic canonical dump for the current
`PreparedMirCoreView` core projection, with focused tests proving included core
facts and excluded diagnostic/prepared-history fields.

## Watchouts

- Do not compare or serialize the whole `prepare::PreparedBirModule`.
- Do not use route text, prepare notes, completed phases, proof text, rendered
  debug output, expectations, unsupported markers, allowlists, or target output
  as equality authority.
- Keep source/freshness authority work in the separate open idea 692 unless the
  supervisor explicitly switches lifecycle state.

## Proof

Activation-only lifecycle slice. No build or CTest required.
