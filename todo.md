Status: Active
Source Idea Path: ideas/open/153_phase_c_prealloc_private_cache_contraction_audit.md
Source Plan Path: plan.md
Current Step ID: Step 1
Current Step Title: Read Prerequisite Phase Artifacts

# Current Packet

## Just Finished

Step 1 of `plan.md` completed the prerequisite read and wrote the route map to
`docs/bir_prealloc_fusion/phase_c_private_cache_contraction.md`.

The map covers Routes 1-7 plus the route index reference facade, including
Phase A/B prerequisite sources, implemented BIR annotation/index surfaces,
residual consumers, oracle coverage, and explicit contraction blockers.

## Suggested Next

Step 2 packet: inventory remaining prealloc surfaces without implementation
changes. Inspect `src/backend/prealloc/prepared_lookups.*`, domain lookup
headers/implementations introduced by ideas 137-150,
`src/backend/prealloc/module.hpp`, prealloc plan files,
`src/backend/prealloc/stack_layout/`, and direct AArch64 consumers that still
include or call prealloc query types. Group each surface by semantic owner,
construction-only cache role, consumer dependency, public header exposure, and
the Route 1-7 or route-index prerequisite that keeps it public or allows later
privatization.

## Watchouts

- This plan is analysis-only; do not perform implementation changes during
  Phase C.
- Route 4 still has a block-entry MIR consumer on the older semantic PHI scan.
- Route 5 still has a current-block join-source public helper on the prior
  semantic implementation.
- Route 6 did not switch broad MIR/codegen/prealloc production consumers.
- The route index facade covers selected Route 4 and Route 7 references only;
  Routes 1, 2, 3, 5, and 6 retain typed record/index shapes.

## Proof

Command: `printf 'analysis-only Step 1; no build/test required\n' > test_after.log`

Result: passed. This was an analysis-only packet; no build or test was required
by the delegated proof. Proof log path: `test_after.log`.
