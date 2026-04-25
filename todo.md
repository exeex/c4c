Status: Active
Source Idea Path: ideas/open/102_hir_struct_method_member_identity_dual_lookup.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Audit record-owner string identity surfaces

# Current Packet

## Just Finished

Lifecycle activation created the active runbook and executor-compatible state for Step 1.

## Suggested Next

Start Step 1 by auditing rendered owner-name and `TypeSpec::tag` consumers, then classify each use as semantic lookup, codegen spelling, diagnostic spelling, bridge-required, or legacy parity proof.

## Watchouts

Do not demote `TypeSpec::tag`, rewrite codegen type naming, downgrade expectations, or add testcase-shaped shortcuts during the audit.

## Proof

Lifecycle-only activation; no implementation validation required.
