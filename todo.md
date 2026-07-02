Status: Active
Source Idea Path: ideas/open/518_bir_core_model_cleanup_umbrella.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Produce Staged Follow-Up Ideas

# Current Packet

## Just Finished

Step 5 - Produce Staged Follow-Up Ideas completed
`docs/bir_core_cleanup/follow_up_ideas.md`.

The follow-up list turns the Step 4 destination map into staged
behavior-preserving cleanup ideas in recommended execution order. Each
follow-up names owned files, non-goals, validation expectations, reviewer
reject signals, and why the slice should preserve behavior. It keeps idea 422
producer work separate, separates body-only route extraction from later
declaration/header extraction, and creates no new source idea files.

## Suggested Next

Delegate Step 6: finalize the analysis and lifecycle handoff. Verify the Step
1-5 artifacts satisfy the source idea acceptance criteria, confirm no
implementation files changed as part of the analysis, run an appropriate
docs/lifecycle sanity check, and update `todo.md` with the final packet summary
and proof.

## Watchouts

- This idea is analysis-only; Step 6 should not move declarations or
  definitions.
- The follow-up list deliberately keeps body-only route extraction ahead of
  declaration/header extraction.
- Idea 422 producer capability remains separate from file-organization cleanup.
- Route6 call publication remains the highest-risk route body extraction and is
  staged only after route1-route5 inputs stabilize.
- Route3 extraction must preserve explicit route6 access to memory-source
  facts and must not move public memory authority into `lir_to_bir/memory/`.
- Public memory provenance and local-array/semantic-GEP header ideas are late
  declaration-only candidates after route body movement and include direction
  are proven.
- Step 6 should check the working tree for accidental implementation or test
  edits before lifecycle handoff.

## Proof

Proof command:

```sh
git diff --check && test -s docs/bir_core_cleanup/follow_up_ideas.md
```

Result: passed.

No implementation files were changed. No additional C++ queries were needed.
No `test_after.log` was produced because the delegated proof command is a
docs/lifecycle sanity check and does not write one.
