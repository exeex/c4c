# AArch64 Module Phoenix Stage 1 Legacy Evidence Extraction

Status: Open
Created: 2026-05-14
Parent Context: ideas/open/224_common_mir_container_and_target_printer_boundary.md

## Intent

Extract the current AArch64 module emitter and module surface into compressed
markdown evidence so the rebuild can treat `module.cpp` as legacy behavior
evidence, not as the replacement design.

Prepared BIR should lower directly to MIR machine nodes in the rebuilt route.
This stage does not design that route yet; it captures what the current
`src/backend/mir/aarch64/module/` implementation actually owns, mixes, hides,
and exposes.

Read this first before doing phoenix-stage work:
`.codex/skills/phoenix-rebuild/SKILL.md`.

## Stage In Sequence

Stage 1 of 4: legacy markdown extraction for the AArch64 module/MIR lowering
rebuild.

## Produces

- `src/backend/mir/aarch64/module/module.cpp.md`, produced from
  `src/backend/mir/aarch64/module/module.cpp`.
- `src/backend/mir/aarch64/module/module.hpp.md`, produced from the single
  non-helper directory index header
  `src/backend/mir/aarch64/module/module.hpp`.
- `src/backend/mir/aarch64/module/module.md`, a directory-level index for the
  extracted module scope.
- A build-preserving handoff note that keeps `module.cpp` available as legacy
  evidence until a later phoenix stage owns replacement implementation and
  build-system rewiring.

## Does Not Yet Own

- Replacement architecture or file layout decisions.
- New implementation files, new real `.hpp` surfaces, or dispatcher rewiring.
- Instruction-selection semantics beyond classifying current responsibilities.
- Broad AArch64 instruction coverage.
- Changes to prepared frame, call, register allocation, spill/reload, storage
  plan, data, or control-flow authority.

## Unlocks

Stage 2, which reviews the extracted markdown set and defines the replacement
layout and handoff contract for draft generation.

## Scope Notes

- Use the phoenix extraction script:
  `python .codex/skills/phoenix-rebuild/scripts/extract_legacy_to_markdown.py 'src/backend/mir/aarch64/module/module.cpp' 'src/backend/mir/aarch64/module/module.hpp'`
- The in-scope directory currently has exactly one non-helper `.hpp`:
  `module.hpp`. `helper.hpp` may exist in future, but it must not replace the
  directory index role.
- Write compressed evidence, not source dumps.
- Capture important APIs, contracts, dependency directions, hidden
  dependencies, responsibility buckets, and special-case classifications.
- Use short fenced `cpp` blocks only for essential surfaces.
- Force every fast path or special case into one of these labels:
  core lowering, optional fast path, legacy compatibility, or overfit to
  reject.
- The directory-level index must point at the full artifact set and summarize
  how the current module surface relates to public AArch64 assembly,
  `api/api.hpp`, codegen records, and prepared BIR inputs.

## Boundaries

- Do not hand-copy full source files into markdown.
- Do not draft replacement APIs in this stage.
- Do not delete `module.cpp` in this stage; teardown belongs to the later
  phoenix stage that first provides a replacement implementation and updates
  build wiring.
- Do not treat current record boundaries as trusted design boundaries.
- Do not weaken or rewrite tests to make extraction look complete.

## Completion Signal

This idea is complete only when every in-scope legacy source has a compressed
markdown companion, `module.hpp` has its `module.hpp.md` index-surface
companion, `module.md` points at the full artifact set, no in-scope directory
has more than one non-helper `.hpp`, Stage 1 explicitly records that
`module.cpp` remains compiled legacy evidence until a later replacement stage
owns teardown plus build rewiring, and reviewers can understand the module
emitter responsibilities from the extracted evidence without treating the live
source as the replacement design.

## Reviewer Reject Signals

Reject the route if extraction is a source dump, omits `module.hpp`, omits the
directory index, deletes or build-disconnects `module.cpp` before replacement
implementation and build wiring exist, silently adds another non-helper header
in the module directory, classifies direct prepared-BIR-to-MIR lowering as out
of scope, or claims progress through testcase-shaped shortcuts, unsupported
expectation downgrades, helper renames, expectation rewrites, or new labels
that preserve the old module-emitter failure mode unchanged.
