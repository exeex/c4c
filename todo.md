Status: Active
Source Idea Path: ideas/open/235_aarch64_scalar_cast_and_float_machine_nodes.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Inspect Scalar Cast And FP Surfaces

# Current Packet

## Just Finished

Activated `ideas/open/235_aarch64_scalar_cast_and_float_machine_nodes.md` as
the active lifecycle runbook.

## Suggested Next

Execute Step 1 from `plan.md`: inspect scalar cast and F32/F64 operation
surfaces, then record the first implementation packet target and focused proof
subset before code changes.

## Watchouts

- Do not rebuild archived accumulator-based cast or FP snippets.
- Do not fabricate GPR/FPR register homes or register-bank transitions in
  AArch64 target lowering.
- Keep F128 and binary128 delegated to the separate binary128 soft-float route.
- Treat expectation-only changes, named-case shortcuts, and F128-as-F64
  lowering as route drift.

## Proof

Lifecycle activation only. No build or test proof was required.
