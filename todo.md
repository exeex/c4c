Status: Active
Source Idea Path: ideas/open/103_prealloc_synthetic_helper_call_abi_authority.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Inventory Synthetic Helper ABI Producers And Consumers

# Current Packet

## Just Finished

Step 1 inventory is complete.

Producer surfaces:

- `src/backend/prealloc/regalloc/runtime_helpers.cpp` is the semantic selector for synthetic helper calls. `append_i128_runtime_helper_mappings` maps BIR `I128` div/rem to `__divti3`, `__udivti3`, `__modti3`, and `__umodti3`, and maps supported `I128` float/integer casts to compiler-rt helper names such as `__fixdfti`, `__fixunsdfti`, `__floattidf`, and `__floatuntidf`. It records source opcode/cast, result and operand prepared value identities, source/result widths/signness, helper family/kind, callee identity, and result ownership.
- The same file's `append_f128_runtime_helper_mappings` maps `F128` arithmetic to `__addtf3`, `__subtf3`, `__multf3`, and `__divtf3`; maps `F128` comparisons to `__eqtf2`, `__netf2`, `__lttf2`, `__letf2`, `__gttf2`, and `__getf2`; and maps `F32/F64 <-> F128` casts to `__extendsftf2`, `__extenddftf2`, `__trunctfsf2`, and `__trunctfdf2`. For comparisons it intentionally records helper `result_type=I32`, BIR result type `I1`, the predicate-specific zero test, and `consumes_helper_cmp_result/owns_bir_i1_result`.
- `src/backend/prealloc/i128_runtime_helpers.cpp` enriches the selected i128 helpers from prepared carrier/value facts. It populates low/high carrier lanes, scalar operand/result ownership for conversion helpers, helper ABI bindings, marshaling moves, resource policy, caller-saved clobbers, live-preservation policy, and `selected_call_ownership`.
- `src/backend/prealloc/f128_runtime_helpers.cpp` enriches f128 helpers from prepared F128 carriers and scalar value homes. It populates carrier/scalar ownership, ABI bindings, marshaling moves, the f128 comparison I32-to-I1 consumption contract, resource policy, caller-saved clobbers, live preservation, and `selected_call_ownership`.
- `src/backend/prealloc/prealloc.cpp` orders helper fact publication after carrier/storage plans: `populate_f128_runtime_helper_facts` and `populate_i128_runtime_helper_lanes` consume already-published carriers, value locations, frame plans, liveness, and regalloc facts. `src/backend/prealloc/regalloc.cpp` seeds the initial helper mappings during regalloc.

Consumer surfaces:

- `src/backend/prealloc/prepared_printer/runtime_helpers.cpp` consumes the prepared helper records for audit output. It prints helper family/kind, opcode/cast, callee, source/result types, ownership, resource policy, ABI transition, clobbers, carriers, ABI bindings, marshaling, scalar ownership, comparison result consumption, live preservation, selected ownership, and missing facts.
- AArch64 MIR helper lowering consumes prepared helper authority through `make_prepared_i128_runtime_helper_boundary_record` and `make_prepared_f128_runtime_helper_boundary_record` in `src/backend/mir/aarch64/codegen/i128_ops.cpp` and `src/backend/mir/aarch64/codegen/f128.cpp`. Dispatch fails closed on missing or incomplete prepared helper authority instead of reconstructing helper ABI from source BIR shape.
- `src/backend/mir/aarch64/codegen/f128.cpp` owns the physical f128 comparison bridge after consuming the prepared semantic contract: the prepared helper says the helper returns an `I32` compare result for a BIR `I1` result with a specific zero-test; AArch64 materializes the I1 register from that `I32` ABI result.

Existing proof surfaces:

- `tests/backend/bir/backend_prepare_liveness_test.cpp` checks i128 helper mapping authority, including div/rem callee identity, source opcode/value authority, GPR-pair ABI policy, lane bindings, marshaling, clobber policy, and terminal-call ownership.
- `tests/backend/bir/backend_prepared_printer_test.cpp` audits the prepared helper dump for i128 and f128 helper facts, including callee, ABI transition, carriers, scalar/cast fields, and selected ownership.
- `tests/backend/mir/backend_aarch64_target_instruction_records_test.cpp` checks that AArch64 records consume prepared i128/f128 helper authority, preserve helper callee/kind/source facts, reject unsupported or incomplete helpers, and preserve f128 comparison `I32` result plus materialized BIR `I1` ownership.
- `tests/backend/mir/backend_aarch64_instruction_dispatch_test.cpp` checks dispatch uses prepared helper authority, fails closed when helper records or selected ownership are missing, and lowers the f128 comparison helper into an I1 materialization using the prepared `cmp_result_consumption`.
- `tests/backend/mir/backend_aarch64_machine_printer_test.cpp` checks final helper call printing for i128/f128 helper boundaries, including comparison and cast cases.

Authority split:

- Semantic helper ABI facts are the prepared helper records: helper family/kind, source opcode/cast, source/result type facts, callee identity, ABI transition, argument/result banks and counts, result ownership, value identities, and the f128 comparison `I32` helper result to BIR `I1` zero-test contract.
- Physical facts remain prealloc/MIR movement authority: low/high i128 lane carriers, f128 full-width/scalar carriers, register and stack homes, ABI register placement, caller-saved clobber sets, live preservation routes, marshaling move records, materialized AArch64 registers, and register-stack placement.
- BIR does not own these synthetic runtime helper calls as direct call instructions. BIR contributes the source operation/cast shape and value types; prealloc selects and publishes the synthetic helper ABI contract, and MIR consumes that prepared contract.

## Suggested Next

Proceed to Step 2 by deciding the narrow contract surface: keep synthetic helper ABI authority as prepared-helper metadata, require consumers to fail closed without it, and decide whether any record fields need tightening for i128 helpers, f128 arithmetic/cast helpers, or the f128 comparison I32-to-I1 bridge.

## Watchouts

- Do not treat synthetic helper calls as source BIR direct calls.
- Do not move caller-saved clobbers, preservation, carrier marshaling, or register/stack movement into BIR.
- Do not let AArch64 lowering infer f128 comparison bridge semantics from callee names or testcase-shaped opcode checks alone; the prepared `ScalarCmpResultConsumption` contract is the named semantic bridge.
- Keep unsupported helper families fail-closed instead of guessing helper callees or ABI transitions.

## Proof

Passed:

`git diff --quiet -- src/backend/prealloc src/backend/bir tests && printf 'analysis-only proof: no implementation or test diff for synthetic helper ABI inventory\n' > test_after.log`

`test_after.log`: analysis-only proof: no implementation or test diff for synthetic helper ABI inventory
