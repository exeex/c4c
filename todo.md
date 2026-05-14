Status: Active
Source Idea Path: ideas/open/227_aarch64_module_phoenix_replacement_drafts.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Draft Replacement Index And Header Contract

# Current Packet

## Just Finished

Stage 3 activation created the active runbook from idea 227 and the accepted
Stage 2 handoff artifacts. No implementation, build, test, or Stage 2 artifact
files were edited.

## Suggested Next

Delegate Step 1 to an executor.

Owned files:

- `src/backend/mir/aarch64/module/module.md`
- `src/backend/mir/aarch64/module/module.hpp.md`

Required read-first files:

- `.codex/skills/phoenix-rebuild/SKILL.md`
- `ideas/open/227_aarch64_module_phoenix_replacement_drafts.md`
- `src/backend/mir/aarch64/module/stage2_review_layout.md`
- `src/backend/mir/aarch64/module/stage2_to_stage3_handoff.md`
- `src/backend/mir/aarch64/module/module.cpp.md`
- `src/backend/mir/aarch64/module/module.hpp.md`
- `src/backend/mir/aarch64/module/module.md`

Packet goal:

- Convert `module.md` and `module.hpp.md` from Stage 1 extraction/evidence
  artifacts into Stage 3 replacement drafts that establish the directory index,
  public header contract, canonical MIR carrier vocabulary, provenance model,
  compatibility-projection boundaries, and target-owned printable surfaces.

## Watchouts

- Stage 3 is draft-only; do not edit real `.cpp`, `.hpp`, build, or test files.
- Do not delete, disable, or build-disconnect the compiled legacy `module.cpp`.
- Do not add component-level public headers. `helper.hpp` is the only allowed
  header exception and must be justified before drafting.
- Do not make `FunctionRecord::machine_nodes`, cached display strings, spelling
  recovery, or broad public inspection records semantic lowering authority.
- Do not introduce a target render API named `__repr__`.
- If the Stage 2 artifact map appears wrong, stop and report a Stage 2 contract
  repair blocker instead of silently adding or removing files.

## Proof

Lifecycle-only activation. No build or test proof required for this packet.
