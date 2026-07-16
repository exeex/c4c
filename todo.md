# Current Packet

Status: Active
Source Idea Path: ideas/open/803_bir_exceptional_control_allocation_and_frame_design_completion.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Define `asm goto` instruction-point SSA snapshots

## Just Finished

- Completed plan Step 2: defined the exact-revision B4/B5 non-local visibility
  and retained-memory contracts, E1 exceptional allocation facts, E2/E3
  assignment and explicit rewrite duties, E4 home/frame closure, cumulative
  fail-closed verification, and one complete B3-through-F1 scenario.

## Suggested Next

- Execute plan Step 3 and define `asm goto` instruction-point SSA snapshots
  while preserving B3's sole ownership of topology and edge occurrences.

## Watchouts

- Documentation-only: do not modify code, tests, build files, scripts,
  generated artifacts, binaries, canonical regression logs, or unrelated
  lifecycle sources.
- Reuse Step 2's exact-key, no-reconstruction, and stage-local fail-closed
  pattern, but do not conflate non-local-return facts with `asm goto` edges.
- Step 3 must preserve duplicate successor-slot occurrence identity and split
  topology (B3) from instruction-point value visibility (B4).

## Proof

- Documentation-only proof: `git diff --check`; Markdown-only changed-path
  audit; changed relative-link validation; and focused audits for registered
  non-local boundaries, exact-revision products, volatile/escape/indeterminate
  memory identity, E1 clobber/reload exposure, E2 prohibition, E3 realization
  and sole retry, E4 home/frame closure, fail-closed verification, and strict
  apply-only F1. No build/runtime test applies; canonical logs were untouched.
