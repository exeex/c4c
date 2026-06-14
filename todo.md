Status: Active
Source Idea Path: ideas/open/249_phase_f3_route6_call_identity_parity_blocker_map.md
Source Plan Path: plan.md
Current Step ID: 6
Current Step Title: Decide Adapter Readiness or Blocker Status

# Current Packet

## Just Finished

Executed Step 6 of `plan.md`: decided adapter readiness for the Route 6 call
identity boundary.

Evidence references used:

- Step 3 x86 matrix from `git show 42e78a773:todo.md`
- Step 4 riscv non-applicability proof from `git show f2feed12f:todo.md`
- Step 5 fail-closed checklist from current predecessor packet
- Source idea acceptance criteria in
  `ideas/open/249_phase_f3_route6_call_identity_parity_blocker_map.md`

Final readiness decision:

A later one-adapter implementation idea is safe only as an x86-bounded Route 6
call argument source-identity adapter. It is not safe to claim broad
direct-call, wrapper, public prepared call-plan, or riscv parity readiness.
Public prepared call-plan authority must remain observable and stable.

Selected fact:

The only candidate semantic authority remains scalar i32 call argument source
identity after Route 6/prepared `source_value_id` agreement. It is x86-bounded
through `find_consumed_scalar_i32_call_argument_source_authority(...)`. The
adapter may read this fact only after all Step 5 agreement gates pass:
prepared argument row exists, `ConsumedPlans` has the shared Route 6
call-use/source index, the BIR argument is named `i32`, Route 6 block/call/callee
/argument identity matches, the Route 6 `ArgumentValue` still matches the BIR
argument object and spelling, Route 6 and prepared both expose equal
`source_value_id`, Route 6 has `source_value_name`, and the prepared i32 value
home remains available and supported.

Required future source-idea shape:

- Implement one x86-local adapter for scalar named-i32 call argument source
  identity after Route 6/prepared `source_value_id` agreement.
- Keep `PreparedBirModule::call_plans`,
  `PreparedFunctionLookups::call_plans`, and `find_prepared_call_plans` public
  and observable.
- Preserve `ConsumedPlans`, wrapper output, route-debug names, fallback names,
  helper/oracle statuses, unsupported rows, and target ABI/register/stack/result
  policy as compatibility or target authority.
- Fail closed for missing, invalid, duplicate/conflict, mismatch, unsupported,
  prepared-only, Route 6-only, fallback, and policy-sensitive rows.
- Make no riscv implementation claim; record riscv as explicitly
  non-applicable until a separate idea introduces a real riscv Route 6
  call-plan consumer.

riscv non-applicability decision:

No bounded riscv consumer currently applies to the selected fact. Prior Step 4
evidence found no `PreparedFunctionLookups::call_plans`,
`PreparedBirModule::call_plans`, `find_prepared_call_plans`, `ConsumedPlans`,
`route6_find_call_argument_source`, or
`route6_call_argument_source_matches_argument_value_record` use under
`src/backend/mir/riscv`. Ordinary riscv call lowering consumes generic call
operands and target-local `CallArgClass`/register/stack policy. Prepared riscv
module emission consumes `PreparedFunctionLookups::edge_publications` plus
Route 5/Route 3 edge-publication agreement gates, not Route 6 call-use/source
identity.

Retained public prepared authority:

Prepared call-plan APIs remain public prepared compatibility authority. They
cannot be deleted, privatized, bypassed, or renamed as route/BIR-owned by the
future x86 adapter. Prepared-only evidence may stay observable and may drive
compatibility rendering, but cannot become Route 6 semantic authority without
Route 6/prepared `source_value_id` agreement.

Blocker status:

Broad x86/riscv parity and public prepared call-plan authority demotion remain
blocked. The blocker is the absence of any current riscv Route 6 call-plan
consumer and the need to keep all public prepared statuses, debug/fallback
names, wrapper output, `ConsumedPlans`, unsupported rows, and ABI/result policy
stable.

## Suggested Next

Supervisor/plan-owner lifecycle decision for source idea 249. The mapped result
is complete enough to either close this blocker-map idea or create the narrow
x86-local implementation idea described above, while leaving any riscv consumer
work to a separate future source idea.

## Watchouts

Do not accept a follow-up that weakens expected strings, renames
helper/status/debug/fallback output, rewrites wrapper baselines, treats
prepared-only evidence as Route 6 agreement, borrows riscv applicability from
x86, or moves ABI/wrapper/result policy into the selected scalar source
identity fact.

## Proof

No build or test proof was run. The delegated packet was analysis-only and
explicitly required no proof log; no `test_after.log` was produced.
