Status: Active
Source Idea Path: ideas/open/586_uniform_target_register_identity_policy.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Audit Current Target Register Identity Policy

# Current Packet

## Just Finished

Activation created the active runbook and execution-state skeleton for
`ideas/open/586_uniform_target_register_identity_policy.md`.

## Suggested Next

Execute Step 1 from `plan.md`: audit the current target register identity
policy across `src/backend/prealloc/target_register_profile.*`, prepared
placement/identity types, and RV64/AArch64/x86 consumers that need ABI
argument/result physical identity.

## Watchouts

- Do not edit implementation files during Step 1 unless the supervisor
  delegates an implementation packet.
- Do not change semantic ABI classification, value freshness authority,
  preservation fallback, move-bundle authority, broad backend lowering, test
  expectations, unsupported markers, allowlists, or runtime-comparison
  behavior under this idea.
- Treat backend-local physical-register rediscovery as a route smell unless it
  is being replaced by or explicitly documented against the shared
  `PreparedTargetRegisterIdentity` policy surface.

## Proof

Activation-only lifecycle change. No build or code validation required.
