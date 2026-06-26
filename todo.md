Status: Active
Source Idea Path: ideas/open/403_prepared_i16_formal_abi_publication.md
Source Plan Path: plan.md
Current Step ID: Step 1
Current Step Title: Confirm Producer-Side I16 Gap

# Current Packet

## Just Finished

Lifecycle switch completed from idea 395 to
`ideas/open/403_prepared_i16_formal_abi_publication.md`. The new active runbook
starts at Step 1 - Confirm Producer-Side I16 Gap.

## Suggested Next

Executor should execute Step 1 by confirming the existing prepared evidence for
`src/20000403-1.c`, inspecting `direct_bir_call_arg_abi_repair()`, and recording
the narrow proof command for the Step 2 repair packet.

## Watchouts

- Keep the repair producer-side in `src/backend/prealloc/legalize.cpp`; do not
  reconstruct missing `I16` formal ABI in RV64 `object_emission.cpp`.
- Do not special-case `src/20000403-1.c`, `seqgt`, `seqgt2`, or `%p.win`.
- The old idea 395 remains open but deactivated; resume it only after this
  producer-side blocker is repaired or explicitly parked.

## Proof

Lifecycle-only switch; no build or backend proof was run by the plan owner.
