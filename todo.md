# Current Packet

Status: Active
Source Idea Path: ideas/open/803_bir_exceptional_control_allocation_and_frame_design_completion.md
Source Plan Path: plan.md
Current Step ID: 6
Current Step Title: Define the E4 frame-object and packing contract

## Just Finished

- Completed plan Step 5: defined complete immutable versioned E1 allocation
  facts, correctness-before-profitability E2 policy with stable total orders
  and finite bounds, and exact E3 realization with sole bounded `E3 -> E1`
  retry. Added a fact-to-decision-to-rewrite trace.

## Suggested Next

- Execute plan Step 6 and define the closed E4 frame-object taxonomy, packing,
  storage-sharing legality, reachability, and verifier evidence.

## Watchouts

- Documentation-only: do not modify code, tests, build files, scripts,
  generated artifacts, binaries, canonical regression logs, or unrelated
  lifecycle sources.
- Keep frame packing in Step 6. Step 5 defines allocation choices and spill
  requests only; it does not choose spill offsets, frame regions, or sharing.

## Proof

- Documentation-only proof: `git diff --check`; Markdown-only changed-path
  audit; introduced relative-link validation; and focused audits for exact
  revision/policy keys, complete E1 facts, correctness versus profitability,
  stable ordering/ties, finite pools, coalescing legality, victim/eviction
  progress and bounds, exact E3 realization, invalidation, verifier failures,
  and the sole `E3 -> E1` retry. No build/runtime test applies; canonical logs
  were untouched.
