Status: Active
Source Idea Path: ideas/open/249_phase_f3_route6_call_identity_parity_blocker_map.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Build the x86 Agreement and Compatibility Matrix

# Current Packet

## Just Finished

Executed Step 3 of `plan.md`: built the x86 agreement and compatibility matrix
for the selected scalar i32 Route 6 call argument source fact.

Evidence commands used:

- `rg -n "find_consumed_scalar_i32_call_argument_source_authority|find_consumed_call_argument_plan|ConsumedPlans|route6_find_call_argument_source|route6_call_argument_source_matches_argument_value_record" src/backend/mir/x86 src/backend/bir`
- `rg -n "wrapper|route-debug|route debug|fallback|helper|oracle|unsupported|call_plans|PreparedBirModule::call_plans|PreparedFunctionLookups::call_plans|find_prepared_call_plans" src/backend/mir/x86 src/backend/bir tests src | head -n 240`
- `sed -n '80,145p' src/backend/mir/x86/x86.hpp`
- `sed -n '3100,3275p' src/backend/mir/x86/module/module.cpp`
- `sed -n '3480,3595p' src/backend/mir/x86/module/module.cpp`
- `sed -n '1,170p' src/backend/mir/x86/debug/debug.cpp`
- `sed -n '1,220p' src/backend/mir/x86/prepared/dispatch.cpp`
- `sed -n '3360,3445p' src/backend/bir/bir.cpp`

Conclusion:

The bounded x86 direct-call consumer can be classified as semantic agreement
for exactly one fact: scalar i32 call argument source identity after Route 6
and prepared `source_value_id` agreement. The agreement consumer is
`find_consumed_scalar_i32_call_argument_source_authority(...)`; its only
positive output is a Route 6-backed `source_name` used by
`append_prepared_direct_extern_call_argument(...)` when looking up the
prepared i32 value home for a direct-call argument move.

This does not transfer public prepared call-plan authority. `ConsumedPlans`,
public `call_plans`, wrapper kind/output, route-debug names, fallback names,
helper/oracle statuses, unsupported rows, direct/indirect callee identity,
move bundles, registers, result moves, and target ABI policy remain prepared
or target compatibility surfaces.

x86 agreement and compatibility matrix:

| x86 row | Evidence | Classification |
| --- | --- | --- |
| `ConsumedPlans::route6_call_use_sources` plus `ConsumedPlans::calls` | `consume_plans(...)` builds both the prepared call-plan pointer and Route 6 call-use/source index before x86 direct-call rendering consumes either one. | Retained compatibility |
| `find_consumed_scalar_i32_call_argument_source_authority(...)` availability gate | Requires a prepared argument row, shared Route 6 source index, named i32 BIR argument, matching Route 6 call key, matching `ArgumentValue` object/spelling, present Route 6 and prepared `source_value_id`, equal ids, and Route 6 `source_value_name`. | Semantic agreement |
| Missing prepared argument row, missing Route 6 index, non-named/non-i32 argument, Route 6 blocked status, source object mismatch, missing id/name, or prepared/source id mismatch | The helper returns `std::nullopt`; direct-call rendering then falls back to the original BIR argument name for prepared home lookup instead of treating Route 6 as authority. Route-debug can print blocked/mismatch names, but that output is diagnostic compatibility. | Blocked public prepared authority |
| `append_prepared_direct_extern_call_argument(...)` source-name selection | For named i32 arguments it uses the Route 6 `source_name` only when the agreement helper returns authority; otherwise it uses `argument.name`. It still calls `require_prepared_i32_value_home(...)` for the actual location. | Semantic agreement |
| Prepared i32 value home lookup and source register/home choice | Even after Route 6 agreement, the selected source name is resolved through prepared value homes; unsupported homes still throw prepared handoff errors. | Retained compatibility |
| Before-call bundle and call argument ABI register | `require_prepared_call_bundle(...)` and `prepared_call_argument_register(...)` own the ABI destination register and fail-closed handoff behavior. Route 6 does not choose the register or bundle. | Target policy |
| Direct extern wrapper kind and direct callee name | Direct-call paths require `DirectExternFixedArity` or `DirectExternVariadic`, require `direct_callee_name`, and compare it to the BIR callee before emission. This preserves wrapper selection and callee spelling outside the source fact. | Retained compatibility |
| Direct-call emitted text | The scalar argument move and `call` instruction are rendered by x86 helpers using prepared homes, ABI register narrowing, and `data.render_asm_symbol_name(...)`; Route 6 only supplies an agreed source identity. | Target policy |
| Pointer string arguments and immediate i32 arguments | String pointers use data-label rendering and immediates render literal moves; they do not consume the selected scalar named-i32 source fact. | Retained compatibility |
| Loaded named i32 reuse in same-module/direct-call renderer | `loaded_i32_values` may emit from an already materialized x86 operand before falling back to prepared direct extern argument rendering. This is local rendering compatibility, not new semantic authority. | Retained compatibility |
| Indirect same-module calls | The same-module path validates prepared indirect wrapper kind, `is_indirect`, and prepared callee value identity before emitting a direct symbol call; it does not consume the selected scalar direct-call source fact. | Retained compatibility |
| After-call bundle, result home, and call-result move | Result transport is still resolved through prepared result homes and `require_prepared_call_result_move(...)`. The selected fact does not own result movement. | Target policy |
| `ConsumedPlans`, `PreparedFunctionLookups::call_plans`, `PreparedBirModule::call_plans`, and `find_prepared_call_plans` public output | These remain public lookup and observable compatibility authority. Step 3 proves only a bounded agreement read, not deletion, privatization, renaming, or weakening of public prepared call-plan APIs. | Blocked public prepared authority |
| Route-debug status and grouped authority output | `debug.cpp` renders `route6 scalar arg status ... status=... gate=...` and grouped prepared register spans as display strings. These names may expose agreement/mismatch/fallback state but remain expected-output compatibility. | Retained compatibility |
| Helper/oracle statuses, fallback names, and unsupported rows | No x86 row inspected authorizes renaming statuses, weakening unsupported behavior, or changing fallback strings. Negative statuses remain compatibility/fail-closed evidence for later steps. | Blocked public prepared authority |
| Prepared module fast-path and helper dispatch status | `classify_module_fast_path(...)` still reports route-local lane/reason strings based on defined-function count, and prepared dispatch remains independent of the scalar source identity fact. | Retained compatibility |

## Suggested Next

Execute Step 4: build the riscv applicability matrix for the same scalar i32
call argument source fact. The key question is whether riscv has a direct
consumer for this fact or should be recorded as explicitly non-applicable with
fail-closed behavior.

## Watchouts

For Step 4, do not infer riscv applicability from the x86 agreement row. The
x86 proof is bounded to a direct-call scalar named-i32 argument source-name
handoff after prepared id agreement. Riscv needs its own direct consumer, or a
specific non-applicability row that keeps public prepared call-plan authority
blocked.

Do not move wrapper kind, callee identity, ABI registers, prepared move
bundles, helper/oracle status names, route-debug names, unsupported rows, or
target fallback strings into the selected semantic fact. Those are preserved
compatibility/policy rows.

## Proof

No build or test proof was run. The delegated packet was analysis-only and
explicitly required no proof log; no `test_after.log` was produced.
