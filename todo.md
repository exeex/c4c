Status: Active
Source Idea Path: ideas/open/243_aarch64_variadic_machine_node_consumption.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Consume Prepared Facts For Aggregate `va_arg`

# Current Packet

## Just Finished

Lifecycle handoff completed: prerequisite idea 246 closed after supplying
`helper_operand_homes.va_arg_aggregate.aggregate_access_plan`, and idea 243 is
reactivated at aggregate `va_arg` selected machine-node consumption.

## Suggested Next

Execute Step 4 by consuming the prepared/shared aggregate access-plan fact in
AArch64 selected lowering. Start with a narrow aggregate `va_arg` path and
preserve fail-closed diagnostics for missing or incomplete prepared authority.

## Watchouts

- Do not reconstruct aggregate source selection, register-save coordinates,
  overflow coordinates, aggregate size/alignment, copy extent, destination
  payload relationships, or `va_list` progression in AArch64 target lowering.
- Do not claim aggregate `va_arg` support through prepared-printer coverage
  alone; selected machine-node records and printer output are required.
- Treat fixture-name matching, expectation-only changes, and unsupported
  downgrades as route drift.
- The fail-closed selected AArch64 diagnostic remains
  `helper_operand_homes.va_arg_aggregate.aggregate_access_plan` until Step 4
  consumes the prepared fact.

## Proof

Lifecycle-only transition. Prerequisite proof was supplied by commits
`2fbc9f171`, `c28ae98bc`, and `f1591bb2b`: focused backend proof passed
139/139 in `test_before.log`, and the supervisor reported accepted full-suite
baseline 3167/3167 for commit `c28ae98bc`.
