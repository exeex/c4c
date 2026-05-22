Status: Active
Source Idea Path: ideas/open/362_aarch64_pointer_derived_load_address_scaling_timeout.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Refresh Pointer-Derived Load/Scaling Proof

# Current Packet

## Just Finished

Activation only. No executor packet has completed for this runbook yet.

## Suggested Next

Execute Step 1 from `plan.md`: build the tree and run the supervisor-selected
focused pointer-derived load/address-scaling proof around memory operand
contracts, materialized pointer writeback guardrails, and AArch64 `00170`,
`00181`, and `00189`.

## Watchouts

- Do not reopen implementation work unless fresh evidence shows an in-scope
  pointer-derived load/address-scaling first bad fact.
- Do not absorb materialized pointer StoreLocal writeback, direct `LoadGlobal`
  select-store handling, recursive formal post-call repairs, semantic string
  loads, frontend admission, ABI composite, or variadic/floating residuals
  into this packet.
- If proof remains green, the next lifecycle decision is close gate versus
  parked/deactivated state, not routine code changes.

## Proof

Not run during activation.
