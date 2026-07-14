# Current Packet

Status: Active
Source Idea Path: ideas/open/744_lir_remaining_ordinary_value_identity_publication.md
Source Plan Path: plan.md
Current Step ID: 7.17
Current Step Title: Publish builtin-ctz call-result/use authority

## Just Finished

- Completed Plan Step 7.16 for only the i32/i64 builtin-ffs `llvm.cttz` call
  and its exact existing add-one lhs use.
- Allocated the call result through `fresh_value`, published a module-owned
  callee `LinkNameId`, native Cttz kind, exact nonvariadic integer/i1 signature
  and matching return/argument refs, and preserved the result ID as the
  add-one lhs.
- Kept the prepared value argument honest monostate SSA or literal Immediate
  presentation when unavailable and published the structural false flag as
  exact i1 `LirIntegerImmediate{0}`.
- Added exact-ID and misleading-display positives plus invalid/duplicate,
  unknown/cross-function, missing/conflicting/unresolved callee, malformed
  signature/count/type/mirror, wrong argument authority, and nonfalse flag
  rejections. The matrix records the exact boundary.

## Suggested Next

- Execute Step 7.17: publish the i32/i64 builtin-ctz Cttz call result through
  its optional narrowing and one later ordinary i32 use.

## Watchouts

- Own only PI's i32/i64 `emit_builtin_ctz_call` route. Reuse native Cttz kind,
  module-owned intrinsic `LinkNameId`, and the exact nonvariadic integer/i1
  signature, but publish the structural zero-undefined flag as exact i1 true.
- Allocate the intrinsic result through `fresh_value`; return that exact ID for
  i32 and preserve it through an authoritative i64-to-i32 Trunc for the wider
  route, then carry the final exact operand into one later ordinary i32 use.
- Preserve arg0's honest compatibility boundary: available native authority
  must be a valid current-function SSA ID, while monostate SSA or Immediate
  presentation does not acquire payload authority from spelling.
- Require native callee/signature/type/count/flag agreement plus unique result
  ownership and exact call-to-Trunc/use edges. Reject invalid/duplicate,
  unknown/cross-function, unresolved callee, wrong-kind, argument, flag,
  signature, endpoint, or narrowing conflicts.
- Accept misleading displays only after authority is proven. Preserve Step
  7.16's false-flag ffs contract; exclude clz/popcount/parity and all other
  intrinsic/builtin/direct/indirect calls, ABI/variadic work, pointer/vector/
  aggregate/object families, CFG/parameters, inline assembly, and BIR.

## Proof

- Fresh `cmake --build --preset default` passed after the Step-7.16 literal
  prepared-value compatibility repair.
- `ctest --test-dir build -R '^(frontend_lir_call_type_ref|positive_sema_ok_call_builtin_runtime_c|llvm_gcc_c_torture_src_builtin_bitops_1_c)$' --output-on-failure > test_after.log`
  passed 3/3: focused authority coverage plus both exact full-guard regressions.
- Focused coverage proves authority-free i32/i64 literal inputs retain
  Immediate presentation, while payload-authoritative literals and unknown SSA
  IDs reject.
- The supervisor's matched full regression guard passed 3033/3033 before and
  after the literal-input repair, with delta 0 passed / 0 failed and no new
  failures.
- `git diff --check` passed for the complete Step-7.16 slice.
