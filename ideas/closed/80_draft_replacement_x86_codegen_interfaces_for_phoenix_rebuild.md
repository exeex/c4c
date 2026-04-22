# Draft Replacement X86 Codegen Interfaces For Phoenix Rebuild

Status: Closed
Created: 2026-04-22
Last-Updated: 2026-04-22
Closed: 2026-04-22
Disposition: Completed by drafting and reviewing the full stage-2-declared
replacement interface tree under `docs/backend/x86_codegen_rebuild/`.
Parent Idea: [79_review_extracted_x86_codegen_subsystem_for_phoenix_rebuild.md](/workspaces/c4c/ideas/closed/79_review_extracted_x86_codegen_subsystem_for_phoenix_rebuild.md)

## Intent

Draft the replacement x86 codegen subsystem as reviewed per-file markdown
interfaces from the stage-2 layout and handoff before any implementation
conversion begins.

## Stage In Sequence

Stage 3 of 4: replacement draft generation and draft review.

## Produces

- the complete stage-2-declared replacement draft set under
  `docs/backend/x86_codegen_rebuild/`
- one `.cpp.md` for every planned replacement implementation file declared in
  `docs/backend/x86_codegen_rebuild_plan.md`
- one `.hpp.md` for every planned replacement header file declared in
  `docs/backend/x86_codegen_rebuild_plan.md`
- any directory-level index `.md` required by the stage-2 replacement layout
- `docs/backend/x86_codegen_rebuild/review.md`

The draft set must:

- follow the exact file layout declared by stage 2 rather than silently
  inventing or dropping planned files
- follow the route constraints and trust/correction guidance declared in
  `docs/backend/x86_codegen_rebuild_handoff.md`
- partition behavior by responsibility and dependency direction instead of by
  arbitrary slices of the old implementation
- state owned inputs, owned outputs, indirect queries, forbidden knowledge,
  and whether each component is core lowering, dispatch, optional fast path,
  or compatibility
- receive an explicit review inside
  `docs/backend/x86_codegen_rebuild/review.md`

## Does Not Yet Own

This stage does not own:

- converting the reviewed drafts into real `.cpp` / `.hpp` files
- changing the stage-2 replacement layout without first repairing stage 2
- proving final runtime correctness for the completed rebuild

## Unlocks

This stage unlocks stage 4 by giving implementation conversion a reviewed,
per-file replacement contract and ownership map instead of another informal
rewrite attempt.

## Scope Notes

The draft set must show how prepared routes consume shared seams instead of
growing a second lowering stack, and it must make the replacement ownership
graph legible at the file level. Stage 3 should treat
`docs/backend/x86_codegen_rebuild_handoff.md` as the explicit intake contract
from idea 79.

## Boundaries

Do not convert the drafts into real source during this stage.

## Completion Signal

This idea is complete when every planned replacement `.cpp` / `.hpp` from the
stage-2 layout has its corresponding reviewed `.cpp.md` / `.hpp.md` artifact
under `docs/backend/x86_codegen_rebuild/`, any required directory-level index
exists, and `docs/backend/x86_codegen_rebuild/review.md` records that the
draft set is coherent enough to drive implementation conversion.

## Closure Note

Closed on 2026-04-22 after the stage-3 runbook satisfied the owned completion
signal:

- `docs/backend/x86_codegen_rebuild/` now contains the complete 37-file
  stage-2 manifest, including `index.md`, `layout.md`, every required
  `.hpp.md`, every required `.cpp.md`, and `review.md`
- the draft set now records explicit owned inputs, owned outputs, allowed
  indirect queries, forbidden knowledge, and role classification across the
  full replacement tree
- `docs/backend/x86_codegen_rebuild/review.md` now records that the package is
  coherent enough to drive stage-4 implementation conversion

This idea therefore closes as complete and hands execution forward to
idea 81, `81_convert_reviewed_x86_codegen_drafts_to_implementation_for_phoenix_rebuild.md`.

## Validation At Closure

Close-time guard on 2026-04-22 reused the existing focused canonical scope:

- `cmake --build --preset default`
- `ctest --test-dir build -j --output-on-failure -R '^c_testsuite_x86_backend_src_00204_c$' > test_after.log`
- `python3 .codex/skills/c4c-regression-guard/scripts/check_monotonic_regression.py --before test_before.log --after test_after.log --allow-non-decreasing-passed`

Result:

- close accepted
- before reported `0` passed / `1` failed / `1` total
- after reported `0` passed / `1` failed / `1` total
- no new failing tests were introduced on the matched scope
