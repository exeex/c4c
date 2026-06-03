Status: Active
Source Idea Path: ideas/open/103_prealloc_synthetic_helper_call_abi_authority.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Classify Helper ABI Authority

# Current Packet

## Just Finished

Step 2 classification is complete.

Chosen authority model:

- Retain synthetic helper ABI authority as prepared-only, structured prealloc metadata. Do not introduce source BIR `CallInst`-like carriers for i128/f128 runtime helpers because these helper calls are legalization artifacts selected after semantic BIR, not source calls.
- Treat `PreparedI128RuntimeHelper` and `PreparedF128RuntimeHelper` as the contract surfaces. The durable semantic ABI facts belong in those prepared records: helper family/kind, source opcode or cast, source/result type facts, callee identity, result ownership, ABI transition, argument/result bank/count/width policy, prepared value identities, and selected-call ownership.
- Keep physical planning under prealloc/MIR: carrier lanes, full-width/scalar homes, ABI register placement, marshaling moves, caller-saved clobbers, live preservation, stack/register placement, and AArch64 materialized registers remain physical movement facts rather than BIR authority.
- AArch64 MIR may consume prepared helper records and validate completeness, but it must fail closed when prepared helper authority is missing or incomplete instead of reconstructing helper ABI from source operation shape alone.

Family classifications:

- i128 helpers: prepared-only helper ABI authority. `append_i128_runtime_helper_mappings` selects div/rem and supported float/integer conversion callees from BIR opcode/cast/type facts; `populate_i128_runtime_helper_lanes` publishes the structured ABI policy, low/high lane ownership, scalar conversion ownership, marshaling, clobber, and selected ownership facts. For Step 3, make sure i128 conversion helpers are either equally reviewable through the prepared contract or explicitly documented as outside the immediate AArch64 boundary path if not yet consumed.
- f128 arithmetic/cast helpers: prepared-only helper ABI authority. `append_f128_runtime_helper_mappings` selects soft-float arithmetic/cast helper callees; `populate_f128_runtime_helper_facts` publishes ABI policy, carrier/scalar ownership, marshaling, clobber, live preservation, and selected ownership. AArch64 record construction consumes those named prepared facts and rejects mismatched helper kind/opcode/cast/type/callee combinations.
- f128 comparison helpers: prepared-only semantic bridge plus MIR physical materialization. The prepared semantic bridge is named as `PreparedF128RuntimeHelper::ScalarCmpResultConsumption`: helper result `cmp_type=I32`, BIR result `bir_result_type=I1`, predicate-specific `zero_test`, `consumes_helper_cmp_result=true`, and `owns_bir_i1_result=true`. AArch64 owns the physical bridge by consuming that contract, reading the `I32` ABI result, and materializing the BIR `I1` register.

Rejected routes:

- Do not model synthetic helpers as source BIR direct calls.
- Do not add BIR-like helper-call carriers unless a later blocker proves prepared metadata cannot express a semantic helper fact. Current evidence shows the prepared records can carry the needed facts.
- Do not move clobber, preservation, carrier marshaling, or register/stack movement out of prealloc.
- Do not rely on extra callee-name parsing, testcase-shaped opcode matching, or dispatch-side ABI guesses as semantic authority.

Implementation targets for Step 3:

- Name or tighten contract helpers around the prepared-only model instead of widening lifecycle scope: i128 helper ABI completeness, f128 helper ABI completeness, and `ScalarCmpResultConsumption` validity are the likely target surfaces.
- Make any remaining implicit i128 conversion ABI contract reviewable in `PreparedI128RuntimeHelper` facts or explicit helper predicates, while keeping physical movement in prealloc.
- Make f128 arithmetic/cast ABI policy checks locally auditable as prepared-helper contract checks.
- Make the f128 comparison bridge contract locally auditable by validating `ScalarCmpResultConsumption` before AArch64 record materialization.

Proof targets for Step 4:

- i128 helper ABI binding: prepared-plan or instruction-record assertions for callee identity, source opcode/cast, ABI transition, argument/result banks/counts/widths, lane or scalar ownership, marshaling, clobber policy, and fail-closed behavior for missing prepared authority.
- f128 helper ABI binding: prepared-plan or instruction-record assertions for arithmetic and representative cast helpers, including callee identity, helper kind/opcode/cast/type agreement, ABI transition, carrier/scalar ownership, ABI registers, selected-call ownership, and fail-closed behavior for mismatched helper facts.
- f128 comparison result bridge: assertions that `ScalarCmpResultConsumption` publishes `I32 -> I1`, predicate zero-test, `consumes_helper_cmp_result`, and `owns_bir_i1_result`, and that AArch64 consumes that contract to materialize the I1 result without using source BIR direct-call authority.

## Suggested Next

Proceed to Step 3 by implementing or tightening the prepared-only contract names/checks at the existing helper-record surfaces, with special attention to the f128 `ScalarCmpResultConsumption` bridge and any i128 conversion helper ABI facts that are less directly consumed than div/rem.

## Watchouts

- Keep the authority surface prepared-only; a structured BIR-like helper-call carrier is rejected for this route unless a concrete interface blocker appears.
- Do not move physical call planning out of prealloc or MIR. The contract can name semantic ABI facts, but clobbers, preservation, carrier movement, and register/stack placement remain physical prepared/MIR facts.
- Step 3 should not be a pure rename. It needs reviewable contract checks or named predicates that make helper ABI authority and the f128 result bridge explicit.
- Unsupported or mismatched helper families should continue to fail closed.

## Proof

Passed:

`git diff --quiet -- src/backend/prealloc src/backend/bir src/backend/mir tests && printf 'analysis-only proof: no implementation or test diff for synthetic helper ABI classification\n' > test_after.log`

`test_after.log`: analysis-only proof: no implementation or test diff for synthetic helper ABI classification
