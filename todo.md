Status: Active
Source Idea Path: ideas/open/249_phase_f3_route6_call_identity_parity_blocker_map.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Define Fail-Closed and Mismatch Requirements

# Current Packet

## Just Finished

Executed Step 5 of `plan.md`: defined the fail-closed and mismatch
requirements for the selected scalar i32 Route 6 call argument source fact.

Evidence commands used:

- `git log --oneline --decorate -8 -- todo.md`
- `git show 8b1379bce:todo.md`
- `git show 42e78a773:todo.md`
- `git show f2feed12f:todo.md`

Selected fact:

The only candidate semantic authority remains scalar i32 call argument source
identity after Route 6/prepared `source_value_id` agreement. It is x86-bounded
through `find_consumed_scalar_i32_call_argument_source_authority(...)`. Public
prepared call-plan surfaces remain compatibility authority:
`PreparedBirModule::call_plans`, `PreparedFunctionLookups::call_plans`,
`find_prepared_call_plans`, `ConsumedPlans`, wrapper output, route-debug
strings, fallback strings, helper/oracle statuses, unsupported rows, and target
ABI/register/stack/result policy.

x86 agreement gate rows that must stay fail-closed:

| Gate row | Required behavior | Stable surface |
| --- | --- | --- |
| Prepared argument row exists for the call instruction and `arg_index` | Missing row yields no Route 6 semantic authority. Do not synthesize or infer an argument plan. | `ConsumedPlans::calls`, public `call_plans`, direct-call fallback |
| Shared `Route6CallUseSourceIndex` exists in `ConsumedPlans` | Missing index yields no Route 6 semantic authority. | `ConsumedPlans`, route-debug blocked/fallback status |
| BIR argument is named `i32` | Non-named or non-i32 arguments do not enter the selected fact family. | Wrapper/direct-call output, fallback to original argument handling |
| Route 6 record matches block, call instruction index, callee spelling, and argument index | Missing/invalid record yields no Route 6 semantic authority. | Route-debug status strings, public prepared call-plan output |
| Route 6 `ArgumentValue` record still matches the BIR argument object and spelling | Object or spelling mismatch yields no authority. | Route-debug mismatch/fallback status, direct-call fallback |
| Route 6 and prepared argument both have `source_value_id` | Missing id on either side yields no authority. | Route-debug `missing_source_value` or missing-prepared-source style status |
| Route 6 `source_value_id` equals prepared argument `source_value_id` | Id mismatch yields no authority. | Route-debug `source_value_mismatch`/`prepared_source_mismatch` style status |
| Route 6 has `source_value_name` | Missing name yields no authority. | Route-debug `missing_source_name`, direct-call fallback |
| Prepared i32 value home for the agreed `source_name` remains available and supported | Unsupported or missing home remains prepared handoff failure/fallback behavior, not Route 6 ownership. | Prepared status/oracle surface, wrapper output stability |

Fail-closed checklist:

| Negative case | Requirement | Compatibility surface that must remain stable |
| --- | --- | --- |
| Missing prepared call-plan aggregate, function call plans, or argument row | Return no selected semantic authority and preserve public prepared lookup behavior. No code path may delete, privatize, or bypass `PreparedBirModule::call_plans`, `PreparedFunctionLookups::call_plans`, or `find_prepared_call_plans`. | Public prepared status and lookup surfaces, `ConsumedPlans::calls` |
| Missing Route 6 call-use/source index or record | Treat Route 6 evidence as unavailable and use existing prepared/BIR fallback behavior. Do not mark the case as agreement. | `ConsumedPlans::route6_call_use_sources`, route-debug blocked/fallback strings |
| Invalid call key, invalid `arg_index`, wrong callee spelling, wrong block, or stale instruction index | Reject Route 6 authority; the direct-call path must not consume a mismatched source name. | Route-debug mismatch status, wrapper/direct-call output |
| Duplicate/conflicting prepared rows or duplicate/conflicting Route 6 source rows | Fail closed to blocked public prepared authority unless existing producers already define a deterministic rejection status. Do not resolve conflicts by picking a convenient row. | Public prepared status/oracle output, route-debug conflict/blocked strings, `ConsumedPlans` |
| Route 6 `ArgumentValue` object or spelling mismatch | Return no authority even if an id happens to match. | Route-debug mismatch/fallback status, direct-call fallback |
| Missing Route 6 `source_value_id`, missing prepared `source_value_id`, or unequal ids | Return no authority. This is the core mismatch gate and cannot be weakened into name-only agreement. | Route-debug `missing_source_value`, `missing_prepared_source`, `source_value_mismatch`, or `prepared_source_mismatch` style strings |
| Missing Route 6 `source_value_name` after id agreement | Return no authority because x86 has no stable source name to hand to prepared home lookup. | Route-debug `missing_source_name`, fallback to original argument name |
| Unsupported argument family: non-named value, non-i32 value, pointer/string literal, immediate i32, aggregate, indirect-only, result move, or ABI-bound case | Keep outside the selected fact family. Existing wrapper, helper, fallback, and unsupported statuses stay compatibility output. | Wrapper output, helper/oracle status, unsupported rows, target fallback strings |
| Prepared-only source fact without Route 6 agreement | Prepared call-plan rows may remain observable and may drive compatibility rendering, but they cannot be reclassified as Route 6 semantic authority. | Public prepared `call_plans`, grouped authority output, `ConsumedPlans` |
| Route 6-only source fact without prepared agreement | Route 6 may be diagnostic evidence, but it cannot select x86 move source homes without matching prepared `source_value_id` and prepared home lookup. | Route-debug strings, direct-call fallback, prepared value-home status |
| Prepared i32 home missing or unsupported after semantic agreement | Preserve prepared fail-closed behavior; Route 6 owns only source identity, not home availability or register/stack placement. | Prepared handoff status, wrapper/direct-call output, helper/oracle statuses |
| Fallback from absent or blocked authority | Use the existing fallback path, currently the original BIR argument name plus prepared home checks for x86 direct-call argument rendering. Fallback names and emitted output must not be renamed to claim progress. | Direct-call fallback strings, wrapper baselines, route-debug fallback names |
| Policy-sensitive callee, wrapper, ABI register, stack layout, variadic FPR count, result home, before/after bundle, helper choice, or emitted text | Remain target/prepared policy. The selected fact may not absorb these rows even when source identity agrees. | Wrapper output, target ABI output, helper/oracle status, public expected strings |
| riscv direct call-plan or Route 6 use | Non-applicable in the current tree. No riscv readiness follows from x86 agreement. Any future riscv use needs a separate implementation idea with a real consumer. | Absence of riscv `call_plans`/Route 6 consumer, prepared edge-publication status |
| riscv prepared emission path | Must keep consuming edge-publication/value-home state only. Route 5/Route 3 edge-publication agreement does not prove Route 6 call identity applicability. | Riscv prepared edge-publication statuses and fallback behavior |
| Baseline or expectation change required to make a negative row pass | Reject the row as route drift. The correct result is blocked authority or a separate idea, not expectation weakening. | Test expectations, wrapper baselines, route-debug/helper/status names |

riscv non-applicability proof to carry forward:

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

Reviewer rejection rule:

Any Step 6 readiness claim must be rejected if it depends on weakening expected
strings, renaming helper/status/debug/fallback output, rewriting wrapper
baselines, treating prepared-only evidence as Route 6 agreement, borrowing
riscv applicability from x86, or moving ABI/wrapper/result policy into the
selected scalar source identity fact.

## Suggested Next

Execute Step 6: decide adapter readiness or blocker status. Compare the x86
agreement gate, the riscv non-applicability proof, and this fail-closed
checklist. If readiness is blocked, record the retained public prepared
authority and the narrow follow-up shape without implementation changes.

## Watchouts

The only positive semantic row is x86 scalar named-i32 source identity after
Route 6/prepared `source_value_id` agreement. Public prepared call-plan APIs,
`ConsumedPlans`, wrappers, route-debug, fallback strings, helper/oracle
statuses, unsupported rows, and ABI/result policy remain compatibility or
target authority.

Riscv remains concrete non-applicability, not parity readiness. A later adapter
claim is unsafe unless it either stays explicitly x86-bounded or a separate
riscv implementation idea introduces and proves a real riscv Route 6 call-plan
consumer.

## Proof

No build or test proof was run. The delegated packet was analysis-only and
explicitly required no proof log; no `test_after.log` was produced.
