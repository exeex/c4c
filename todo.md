Status: Active
Source Idea Path: ideas/open/362_aarch64_pointer_derived_load_address_scaling_timeout.md
Source Plan Path: plan.md
Current Step ID: Step 1
Current Step Title: Refresh Pointer-Derived Load/Scaling Proof

# Current Packet

## Just Finished

Lifecycle activation created this runbook from `ideas/open/362_aarch64_pointer_derived_load_address_scaling_timeout.md`.

## Suggested Next

Run `plan.md` Step 1: refresh the focused pointer-derived load/address-scaling proof and record whether a live in-scope first bad fact remains.

## Watchouts

- Do not treat green focused proof as automatic source-idea closure; closure still requires the close gate.
- Do not reopen implementation work unless fresh evidence shows an in-scope pointer-derived load/address-scaling first bad fact.
- Do not absorb materialized pointer StoreLocal writeback, direct `LoadGlobal` select-store handling, recursive formal post-call repairs, semantic string loads, frontend admission, ABI composite work, or variadic/floating residuals into this packet.
- Do not weaken expectations, unsupported status, runner behavior, timeout policy, CTest registration, or proof-log policy.

## Proof

Pending executor refresh.
