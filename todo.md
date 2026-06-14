Status: Active
Source Idea Path: ideas/open/249_phase_f3_route6_call_identity_parity_blocker_map.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Name the Route 6 Semantic Fact

# Current Packet

## Just Finished

Executed Step 2 of `plan.md`: named the narrow Route 6 semantic fact for the
current x86 agreement path and separated the required prepared agreement rows
from retained prepared/target compatibility output.

Evidence commands used:

- `sed -n '1840,1975p' src/backend/bir/bir.hpp`
- `sed -n '2610,2685p' src/backend/bir/bir.hpp`
- `sed -n '80,145p' src/backend/mir/x86/x86.hpp`
- `sed -n '3100,3275p' src/backend/mir/x86/module/module.cpp`
- `sed -n '3480,3595p' src/backend/mir/x86/module/module.cpp`

Candidate semantic authority:

The narrow Route 6 semantic fact is scalar i32 call argument source identity:
for one BIR call argument at `(block, call instruction index, callee,
arg_index)`, `route6_find_call_argument_source(...)` returns an available
`Route6CallArgumentSourceRecord` whose call/argument relationship still
matches the BIR argument object and spelling, whose `source_value_id` names
the same source value as the prepared call argument plan, and whose
`source_value_name` names the source value that x86 may use for the scalar i32
argument home lookup.

This fact is caller/source/use identity only. It does not own ABI registers,
stack layout, wrapper kind, direct or indirect callee selection, helper choice,
move bundles, emitted text, grouped authority output, route-debug labels, or
call-result move policy.

Current x86 agreement gate:

`find_consumed_scalar_i32_call_argument_source_authority(...)` is the bounded
x86 consumer. It succeeds only when all of these semantic/prepared agreement
conditions hold:

- The prepared argument row exists via `find_consumed_call_argument_plan(...)`.
- A shared `Route6CallUseSourceIndex` exists in `ConsumedPlans`.
- The BIR argument is a named `i32` value.
- `route6_find_call_argument_source(...)` finds the Route 6 record for the
  same block, call instruction index, callee spelling, and argument index.
- `route6_call_argument_source_matches_argument_value_record(...)` confirms
  the Route 6 `ArgumentValue` record still names the same call argument object
  and spelling.
- Route 6 and the prepared argument both have `source_value_id`.
- The Route 6 `source_value_id` equals the prepared argument
  `source_value_id`.
- Route 6 has `source_value_name`, which becomes the returned authority
  `source_name`.

Prepared call-plan answers that must agree with the semantic fact or fail
closed:

| Prepared answer | Required relation to Route 6 fact | Classification |
| --- | --- | --- |
| Per-call argument row existence and `arg_index` | Must find the same prepared argument row for the call instruction and argument index; missing prepared row fails closed to no semantic authority. | Semantic agreement gate |
| Prepared argument `source_value_id` | Must be present and equal to Route 6 `source_value_id`; missing or mismatched id fails closed. | Semantic agreement gate |
| Prepared i32 value home for returned `source_name` | May be queried only after Route 6/prepared id agreement. It chooses the actual prepared home for rendering the x86 move and remains prepared compatibility/policy. | Prepared compatibility after agreement |
| Prepared before-call bundle and call argument register | Required to emit x86 argument moves, but it does not define the source identity fact. Missing bundle/register remains prepared fail-closed handoff behavior. | Target compatibility |
| Prepared direct callee name and wrapper kind | Must remain stable for wrapper selection and direct-call eligibility, but it is not the Route 6 call-use/source fact. | Prepared/target compatibility |
| Prepared indirect callee identity | Must remain stable for same-module indirect wrapper eligibility, but it is not the selected scalar argument source fact. | Prepared/target compatibility |
| Prepared after-call bundle, result home, and call-result moves | Must remain prepared authority for result transport; Step 2 does not transfer call-result move ownership to Route 6. | Prepared/target compatibility |
| Public `PreparedBirModule::call_plans`, `PreparedFunctionLookups::call_plans`, and `find_prepared_call_plans` answers | Must remain observable mirrors and lookup sources for compatibility. Public prepared call-plan authority is not deleted, privatized, or weakened by the selected fact. | Retained public compatibility |
| Grouped authority output and route-debug strings | May report agreement/mismatch/fallback status, but strings and expected output are compatibility surface, not semantic ownership transfer. | Prepared/target compatibility |
| ABI registers, wrapper text, move bundle contents, stack layout, and helper choice | Must stay target policy even when the scalar source identity fact agrees. | Target policy |

## Suggested Next

Execute Step 3: build the x86 agreement and compatibility matrix for the
selected scalar i32 call argument source fact. The matrix should decide whether
the existing x86 direct-call consumer can be classified as semantic agreement
while preserving prepared call-plan output and target policy.

## Watchouts

For Step 3, keep the agreement claim narrower than "x86 calls use Route 6".
The only named candidate here is scalar i32 call argument source identity after
prepared `source_value_id` agreement. Direct and indirect callee identity, ABI
registers, before/after move bundles, call-result moves, wrapper kind/text,
grouped authority output, and route-debug strings remain prepared/target
compatibility.

The Route 6 status enum includes ABI-bound and duplicate/missing cases, but
this Step 2 fact does not move ABI-bound policy into Route 6. Those statuses
should be treated as fail-closed diagnostics or negative proof rows in later
steps, not positive semantic authority for wrapper/register behavior.

## Proof

No build or test proof was run. The delegated packet was analysis-only and
explicitly required no proof log; no `test_after.log` was produced.
