# Current Packet

Status: Active
Source Idea Path: ideas/open/732_bir_stage_document_convergence_umbrella.md
Source Plan Path: plan.md
Current Step ID: 6
Current Step Title: Plan phase E allocation and MIR-ready publication

## Just Finished

- Completed plan Step 5: converged D1-D5; D4 is the mandatory target
  realizability/explicit-expansion owner, and D5 alone removes dynamic SSA into
  allocation-visible edge transfers/scratch before its post-E3 private copy
  resolution handoff to E4.

## Suggested Next

- Execute plan Step 6 in E1-E4 order: exact liveness/interference, shared
  abstract allocation, explicit spill/reload with bounded E3-to-E1 retry, copy/
  frame closure, final product recomputation, and atomic Allocated/MIR-ready
  publication.

## Watchouts

- E1 must observe every D5-introduced value/def/use/clobber/copy/scratch and use
  the exact initial-D5 revision/projection.
- E2 cannot spill `CopyScratch`; E3 alone inserts explicit spill/reload and each
  mutation returns to E1 with a fresh projection/product chain.
- Post-stable-E3 D5 copy resolution remains private input to E4 and cannot
  change assignments or publish an intermediate capability.
- Markdown-only output; proposed code belongs only in fenced Markdown blocks.

## Proof

- Changed-path suffix audit: every changed path ends in `.md`.
- Structural checks confirm D1, D2, D4, and D5 each contain the exact 17-section
  pass spine and closed matrix; D1-D5 revision/projection/gate adjacency is exact.
- D4 output states zero expansion placeholders; initial D5 output states zero
  pending phi and allocation-visible scratch; subordinate D5 output states no
  unresolved parallel/scratch and a private E4-only handoff.
- `git diff --check` passes. No build/test run is applicable to this
  documentation-only packet; no `test_after.log` is created or modified.
