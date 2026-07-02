Status: Active
Source Idea Path: ideas/open/553_move_bundle_target_shape_evidence_gap_src_960209_1.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Reproduce The Evidence Gap

# Current Packet

## Just Finished

Lifecycle activation created the runbook for
`ideas/open/553_move_bundle_target_shape_evidence_gap_src_960209_1.md`.

## Suggested Next

Execute Step 1, "Reproduce The Evidence Gap": build a one-row allowlist for
`src/960209-1.c`, run the focused backend scan, and record the current
diagnostic plus missing facts before any implementation changes.

## Watchouts

- This plan is evidence-first. Do not route the row to RV64, prepared, BIR, or
  F128 from filename, source shape, raw BIR shape, or bucket membership alone.
- Do not change expectations, unsupported markers, allowlists, or runtime
  comparison behavior.
- Keep any diagnostic work focused on emitting auditable facts, not on making a
  narrow testcase pass.

## Proof

Activation-only lifecycle change. No build proof was required.
