Status: Active
Source Idea Path: ideas/open/198_phase_d_phase_e_lifecycle_naming_cleanup.md
Source Plan Path: plan.md
Current Step ID: Step 4
Current Step Title: Write the Durable Naming Note

# Current Packet

## Just Finished

Step 4 durable naming-note artifact is complete for idea 198, `Phase D versus
Phase E lifecycle naming cleanup`.

Created
`docs/bir_prealloc_fusion/phase_d_phase_e_lifecycle_naming_cleanup.md`.
The artifact classifies ideas 182-189 as Phase D follow-up slices despite
their historical `phase_e_*` closed filenames, includes a table for each idea
with actual accepted scope, retained/not-proven scope, and recommended future
citation wording, and states explicitly that no closed files were renamed.

The note also records that draft 155 remains the future true Phase E
`PreparedBirModule` retirement analysis. It names the prerequisite docs that
should exist before draft 155 activation or rewrite: readiness audit, residual
consumer migration map, cross-target reuse map, `PreparedFunctionLookups`
ownership map, diagnostics/oracle replacement plan, return-chain import note,
and relevant Phase A-D / return-chain background docs.

Stable citation language and reject-language rules from Step 3 were copied
into the durable note. The note rejects claims of implementation progress,
prepared API contraction, broad `PreparedFunctionLookups` contraction, broad
`PreparedBirModule` retirement, draft 155 readiness, route-wide migration,
broad x86/riscv readiness, target-policy migration, expectation progress, and
unsupported-marker progress.

## Suggested Next

Execute Step 5 final consistency check. Confirm the durable naming note
satisfies idea 198 acceptance criteria, diff scope is docs/lifecycle-only,
Markdown whitespace is clean, and the source idea is ready for plan-owner
close if Step 5 passes.

## Watchouts

- Preserve historical closed idea filenames and contents.
- Do not open, rewrite, or execute draft 155 under this naming-cleanup plan.
- Step 5 should not edit implementation, tests, expectations, unsupported
  markers, prepared API contracts, source ideas, or closed history.
- Treat the artifact as a citation clarification only; it does not prove
  `PreparedBirModule` retirement, draft 155 readiness, broad
  `PreparedFunctionLookups` contraction, route-wide migration, broad x86/riscv
  readiness, or target-policy migration.
- Verify the table still names retained/not-proven scope for every idea
  182-189 before closing.

## Proof

Docs/lifecycle-only artifact packet. Inspected the active plan, source idea
198, current `todo.md`, closed ideas 182-189, draft 155, and prerequisite
readiness/cross-target/ownership/diagnostics/return-chain docs with targeted
`sed` and `rg` searches. No build was required and no `test_after.log` was
generated for this packet. Before handoff, ran `git diff --check -- todo.md
docs/bir_prealloc_fusion/phase_d_phase_e_lifecycle_naming_cleanup.md` and
targeted `rg` checks for the required artifact language.
