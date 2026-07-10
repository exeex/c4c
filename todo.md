Status: Active
Source Idea Path: ideas/open/660_rv64_pointer_local_lowering_route_runtime.md
Source Plan Path: plan.md
Current Step ID: Step 1
Current Step Title: Refresh Pointer-Local Evidence

# Current Packet

## Just Finished

Lifecycle activation created the active runbook for Step 1 of
`ideas/open/660_rv64_pointer_local_lowering_route_runtime.md`.

## Suggested Next

Delegate Step 1 evidence refresh for the six focused pointer-local route and
runtime rows. The executor should record current failure boundaries and name
the first owner or split before implementation.

## Watchouts

- Do not reopen idea 657's older `loop-2e.c` route as the first owner unless
  fresh focused evidence proves the representative pass regressed.
- Do not change expectations, unsupported markers, allowlists, runtime policy,
  timeout policy, or baseline accounting.
- Keep byval, destination-publication, object-data, packed-member, AArch64,
  prepared CLI, RISC-V object emission, and LLVM torture work out of this
  packet.

## Proof

Lifecycle-only activation. No build or CTest run was required by this packet.
