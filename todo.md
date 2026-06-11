Status: Active
Source Idea Path: ideas/open/198_phase_d_phase_e_lifecycle_naming_cleanup.md
Source Plan Path: plan.md
Current Step ID: Step 3
Current Step Title: Define Stable Citation Language

# Current Packet

## Just Finished

Step 3 citation-language definition is complete for idea 198, `Phase D versus
Phase E lifecycle naming cleanup`.

Accepted language for ideas 182-188:

- Preferred umbrella phrase: "Phase D follow-up selected-consumer migration
  with a Phase E-like filename."
- Also acceptable: "Phase D follow-up evidence for one selected Route N
  consumer boundary, stored under a historical `phase_e_*` closed idea name."
- When naming the work item, include the proven boundary and retained fallback:
  "idea 18N proves one selected AArch64 Route N consumer boundary and preserves
  prepared fallback plus target/debug/oracle surfaces."
- When citing the group, write: "ideas 182-188 are useful inputs for future
  Phase E planning, but they are not the Phase E retirement plan."

Accepted language for idea 189:

- Preferred phrase: "Phase D follow-up cross-target interface reuse with a
  Phase E-like filename."
- More precise citation: "idea 189 proves one x86 Route 6 scalar call-use
  source interface reuse point after AArch64 Route 6 proof, with prepared
  fallback for absent or mismatched facts."
- Always pair idea 189 with the boundary limits: it does not prove route-wide
  x86 readiness, any riscv readiness, or target-policy migration. x86 ABI,
  frame, register, wrapper, move, formatting, and emission policy remain
  target-local; riscv has no ready route-view reuse point from ideas 182-189.

When future docs should cite the durable naming note:

- Cite the durable naming note whenever a plan, audit, or closure note refers
  to ideas 182-189 as a group, references their `phase_e_*` filenames, or might
  otherwise let "Phase E" be read as completed `PreparedBirModule` retirement.
- Cite the note instead of repeating the full historical explanation when the
  only needed fact is lifecycle meaning: "these are Phase D follow-up slices
  with Phase E-like filenames."
- Repeat the detailed explanation only when the document needs the exact route
  boundary, retained fallback, or draft 155 prerequisite status.
- In draft 155 planning, cite the naming note before using ideas 182-189 as
  prerequisites so the activation text does not inherit the misleading filename
  label.

Reject-language examples for future citations:

- Do not call ideas 182-189 "true Phase E retirement", "Phase E retirement
  completion", "the completed Phase E plan", or "draft 155 execution".
- Do not describe them as `PreparedBirModule` contraction, demotion, deletion,
  retirement, replacement by a BIR-owned aggregate, or field-level ownership
  resolution.
- Do not say they complete draft 155, unblock draft 155 automatically, or make
  draft 155 activation safe without the prerequisite readiness, ownership,
  diagnostics/oracle, cross-target, residual-consumer, and return-chain maps.
- Do not call ideas 182-188 route-wide migrations, complete Route 1-7
  migration, residual-consumer exhaustion, prepared API deletion, prepared
  helper removal, or oracle/printer/debug retirement.
- Do not call idea 189 broad x86 readiness, broad cross-target readiness,
  riscv readiness, x86/riscv wrapper migration, or target-policy migration.
- Do not cite label cleanup as implementation progress, test expectation
  progress, unsupported-marker progress, or permission to weaken prepared
  fallback/oracle contracts.
- Do not propose renaming historical closed idea files as routine cleanup; the
  filenames remain historical review artifacts.

## Suggested Next

Execute Step 4: write the durable naming note at
`docs/bir_prealloc_fusion/phase_d_phase_e_lifecycle_naming_cleanup.md`. The
artifact should include a table for ideas 182-189, the stable citation
language above, the draft 155 boundary, the prerequisite docs list, and an
explicit statement that no closed idea files were renamed.

## Watchouts

- Preserve historical closed idea filenames and contents.
- Do not open, rewrite, or execute draft 155 under this naming-cleanup plan.
- Do not edit implementation, tests, expectations, unsupported markers, or
  prepared API contracts.
- Do not claim `PreparedBirModule` retirement progress from selected-consumer
  migrations or label cleanup.
- Treat bare `Phase E` references to ideas 182-189 as citation hazards unless
  they also say "Phase D follow-up" or "Phase E-like filename".
- Do not use idea 189 as evidence for broad x86 or riscv migration readiness;
  it proves only one x86 Route 6 scalar argument-source interface boundary.
- Do not treat the prerequisite maps as permission to remove prepared fallback,
  diagnostics, target-wrapper surfaces, or oracle tests; they mostly classify
  blockers and split future work.
- The durable note should summarize citation rules rather than reopening
  implementation scope or editing closed history.

## Proof

Docs/lifecycle-only citation-language packet. Inspected the active plan,
source idea 198, current `todo.md`, readiness docs, cross-target reuse map,
and closed ideas 182-189 with targeted `sed` and `rg` searches. Ran
`git diff --check -- todo.md`. No build was required and no `test_after.log`
was generated.
