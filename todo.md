# Current Packet

Status: Active
Source Idea Path: ideas/open/744_lir_remaining_ordinary_value_identity_publication.md
Source Plan Path: plan.md
Current Step ID: 7.18
Current Step Title: Publish builtin-clz call-result/use authority

## Just Finished

- Completed Plan Step 7.17 for only PI's i32/i64 builtin-ctz `llvm.cttz`
  call, optional i64-to-i32 Trunc, and one later ordinary i32 use.
- Reused native Cttz and added an explicit undefined-zero semantic fact, exact
  i1 true, module-owned callee ID, and exact fixed integer/i1 signature while
  preserving Step 7.16's defined-zero/false ffs contract.
- Allocated the call result through `fresh_value`; returned it directly for
  i32 and preserved it through a fresh authoritative exact i64-to-i32 Trunc
  for i64, carrying the final exact ID into the later Add.
- Added source/literal width coverage, misleading-display positives, and
  malformed zero behavior, flag, result, trunc-source/kind/endpoint, final-use,
  and literal-payload authority rejections. The matrix records the boundary.

## Suggested Next

- Execute Step 7.18: publish the i32/i64 builtin-clz Ctlz call result through
  its optional narrowing and one later ordinary i32 use.

## Watchouts

- Own only PI's i32/i64 `emit_builtin_clz_call` route. Add and publish native
  Ctlz semantic kind, module-owned intrinsic `LinkNameId`, exact nonvariadic
  integer/i1 signature, and exact undefined-zero i1 true fact without
  callee-name matching.
- Allocate the intrinsic result through `fresh_value`; return that exact ID for
  i32 and preserve it through an authoritative i64-to-i32 Trunc for the wider
  route, then carry the final exact operand into one later ordinary i32 use.
- Preserve arg0's honest compatibility boundary: available native authority
  must be a valid current-function SSA ID, while monostate SSA or Immediate
  presentation does not acquire payload authority from spelling.
- Require native kind/callee/signature/type/count/zero-behavior agreement plus
  unique result ownership and exact call-to-Trunc/use edges. Reject invalid or
  duplicate results, unknown/cross-function uses, unresolved callees, and
  wrong-kind, argument, flag, signature, endpoint, or narrowing conflicts.
- Accept misleading displays only after authority is proven. Preserve the Cttz
  ffs/ctz contracts; exclude popcount/parity and all other intrinsic/builtin/
  direct/indirect calls, ABI/variadic work, pointer/vector/aggregate/object
  families, CFG/parameters, inline assembly, and BIR.

## Proof

- Fresh `cmake --build --preset default` passed for the Step-7.17 production
  changes.
- `ctest --test-dir build -R '^frontend_lir_call_type_ref$' --output-on-failure`
  passed 1/1 after focused identity/display/malformed coverage was added.
- Matched focused `test_before.log` / `test_after.log` passed 1/1 before and
  after with delta 0 passed / 0 failed and no new failures.
- The supervisor's matched full regression guard passed 3033/3033 before and
  after, with delta 0 passed / 0 failed and no new failures.
- `git diff --check` passed. The final commit remains supervisor-owned.
