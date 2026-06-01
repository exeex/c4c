Status: Active
Source Idea Path: ideas/open/71_aarch64_scalar_control_flow_prepared_authority_cleanup.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Map Scalar And Control-Flow Authority Duplication

# Current Packet

## Just Finished

Activated `plan.md` from
`ideas/open/71_aarch64_scalar_control_flow_prepared_authority_cleanup.md`.

## Suggested Next

Begin `plan.md` Step 1 by mapping scalar, cast, comparison, and branch
authority duplication in `alu.cpp`, `cast_ops.cpp`, and `comparison.cpp`, then
record the first concrete executor packet and delegated proof command here.

## Watchouts

Do not fold local register-view helpers under this route; that belongs to
`ideas/open/74_aarch64_local_scalar_register_helper_fold_back.md`. Keep
AArch64 immediate admissibility, materialization, condition-code spelling, and
machine-record construction target-local.

## Proof

Lifecycle activation only; no build or CTest proof required.
