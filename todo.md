# Current Packet

Status: Active
Source Idea Path: ideas/open/744_lir_remaining_ordinary_value_identity_publication.md
Source Plan Path: plan.md
Current Step ID: 7.19
Current Step Title: Publish builtin-popcount call-result/use authority

## Just Finished

- Completed Plan Step 7.18 for only PI's i32/i64 builtin-clz `llvm.ctlz`
  call, optional i64-to-i32 Trunc, and one later ordinary i32 use.
- Added native Ctlz and generalized the existing Cttz zero behavior into a
  shared zero-count fact, publishing exact undefined-zero i1 true, module-owned
  callee ID, and exact fixed integer/i1 signature while preserving both Cttz
  contracts.
- Allocated the call result through `fresh_value`; returned it directly for
  i32 and preserved it through a fresh authoritative exact i64-to-i32 Trunc
  for i64, carrying the final exact ID into the later Add.
- Added source/literal width coverage, misleading-display positives, and
  malformed intrinsic kind, zero behavior, flag, result, trunc-source/kind/
  endpoint, final-use, and literal-payload authority rejections. The matrix
  records the boundary.

## Suggested Next

- Execute Step 7.19: publish the i32/i64 builtin-popcount Ctpop call result
  through its optional narrowing and one later ordinary i32 use.

## Watchouts

- Own only PI's i32/i64 `emit_builtin_popcount_call` route. Add and publish
  native Ctpop semantic kind, module-owned intrinsic `LinkNameId`, and an exact
  nonvariadic one-integer-parameter signature without callee-name matching.
- Allocate the intrinsic result through `fresh_value`; return that exact ID for
  i32 and preserve it through an authoritative i64-to-i32 Trunc for the wider
  route, then carry the final exact operand into one later ordinary i32 use.
- Preserve arg0's honest compatibility boundary: available native authority
  must be a valid current-function SSA ID, while monostate SSA or Immediate
  presentation does not acquire payload authority from spelling.
- Require native kind/callee/signature/type/count agreement plus unique result
  ownership and exact call-to-Trunc/use edges. Reject invalid/duplicate,
  unknown/cross-function, unresolved-callee, wrong-kind, argument, signature,
  endpoint, or narrowing conflicts; Ctpop must carry no zero-count behavior.
- Accept misleading displays only after authority is proven. Preserve Cttz and
  Ctlz contracts; exclude parity and all other intrinsic/builtin/direct/
  indirect calls, ABI/variadic work, pointer/vector/aggregate/object families,
  CFG/parameters, inline assembly, and BIR.

## Proof

- Fresh `cmake --build --preset default` passed for the Step-7.18 production
  changes.
- `ctest --test-dir build -R '^frontend_lir_call_type_ref$' --output-on-failure`
  passed 1/1 after focused identity/display/malformed coverage was added.
- Matched focused `test_before.log` / `test_after.log` passed 1/1 before and
  after with delta 0 passed / 0 failed and no new failures.
- The supervisor's matched full regression guard passed 3033/3033 before and
  after, with delta 0 passed / 0 failed and no new failures.
- `git diff --check` passed. The final commit remains supervisor-owned.
