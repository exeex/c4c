Status: Active
Source Idea Path: ideas/open/237_aarch64_binary128_softfloat_lowering.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Reconcile Parent Binary128 Route With Constant Carrier

# Current Packet

## Just Finished

Lifecycle activation created the follow-on active runbook for Step 1. No
implementation packet has run yet.

## Suggested Next

Delegate Step 1 to an executor with a narrow backend proof command covering
F128 constant-carrier integration with the parent binary128 route and existing
fail-closed diagnostics for missing or scalar-only payloads.

## Watchouts

- Treat closed idea 241 as the full-width F128 constant payload authority.
- Do not reintroduce `F64`, `double`, `immediate_bits`, single-lane payload, or
  target-local reconstruction shortcuts for F128 constants.
- Keep atomic, intrinsic, inline-assembly, scalar FP, and i128 routes outside
  this parent-route follow-on.
- Do not claim closure through expectation downgrades or narrow named-case
  matching.

## Proof

Lifecycle-only activation; no build or test proof was required.
