# Current Packet

Status: Active
Source Idea Path: ideas/open/832_hir_aggregate_owner_function_parameter_crash_repair.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: State the aggregate-owner parameter-lowering invariant

## Just Finished

- 831 Step 1 is accepted in `ba7958ee4`: it established distinct HIR
  aggregate-owner and truthiness-LHS verifier owners from the exact 14/14
  failure reproduction. The HIR route is ordered first.

## Suggested Next

- Diagnose the named HIR path through `lir_owned_type_spec` and
  `populate_lir_function_params` to the invalid
  `typespec_aggregate_owner_key` fact. Define the smallest native ownership
  relation to repair; do not alter code in the truthiness or 830 routes.

## Watchouts

- Do not add a null/default owner fallback, suppress the crash, weaken tests,
  or claim baseline clearance. 833 remains parked until this route returns
  accepted focused proof to 831.

## Proof

- Evidence predecessor: `ba7958ee4` records the fresh exact 14/14 subset
  failure and the clean backend-enabled `f0fc85e4f^` HIR crash provenance.
  Step 1 must retain that provenance while narrowing the HIR owner seam.
