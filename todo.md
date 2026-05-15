Status: Active
Source Idea Path: ideas/open/243_aarch64_variadic_machine_node_consumption.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Consume Prepared Facts For Scalar `va_arg`

# Current Packet

## Just Finished

Lifecycle handoff completed: prerequisite idea 245 closed after supplying
`helper_operand_homes.va_arg.scalar_access_plan`, and idea 243 is reactivated
at scalar `va_arg` selected machine-node consumption.

## Suggested Next

Execute Step 3 by consuming the prepared/shared scalar access-plan fact in
AArch64 selected lowering. Start with the narrow scalar `va_arg` path and
preserve fail-closed diagnostics for missing or incomplete prepared authority.

## Watchouts

- Do not reconstruct GP/FP/overflow source selection, scalar size/alignment,
  result-home relationships, or `va_list` progression in AArch64 target
  lowering.
- Do not claim scalar `va_arg` support through prepared-printer coverage alone;
  selected machine-node records and printer output are required.
- Treat fixture-name matching, expectation-only changes, and unsupported
  downgrades as route drift.

## Proof

Lifecycle-only transition. Prerequisite proof was supplied for commit
`770a457cf`: focused backend proof 139/139 and accepted full-suite baseline
3167/3167.
