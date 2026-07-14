# Current Packet

Status: Packet Complete - Supervisor Proof Pending
Source Idea Path: ideas/open/744_lir_remaining_ordinary_value_identity_publication.md
Source Plan Path: plan.md
Current Step ID: 7.19
Current Step Title: Publish builtin-popcount call-result/use authority

## Just Finished

- Completed Plan Step 7.19 for only PI's i32/i64 builtin-popcount `llvm.ctpop`
  call, optional i64-to-i32 Trunc, and one later ordinary i32 use.
- Added native Ctpop, a module-owned callee ID, and an exact fixed
  one-integer-parameter signature while requiring zero-count behavior to be
  absent and preserving the existing Cttz/Ctlz contracts.
- Allocated the call result through `fresh_value`; returned it directly for
  i32 and preserved it through a fresh authoritative exact i64-to-i32 Trunc
  for i64, carrying the final exact ID into the later Add.
- Added source/literal width coverage, misleading-display positives, and
  malformed intrinsic kind, forbidden zero behavior, signature/count, result,
  trunc-source/kind/endpoint, final-use, and literal-payload authority
  rejections. The matrix records the boundary.

## Suggested Next

- Supervisor: run broader proof, review the packet diff, and choose the next
  bounded Step-7 row through plan-owner.

## Watchouts

- Own only PI's i32/i64 `emit_builtin_popcount_call` route. Native Ctpop uses
  the common intrinsic kind field but must not carry `zero_count_behavior`.
- Preserve arg0's honest compatibility boundary: available native authority
  must be a valid current-function SSA ID, while monostate SSA or Immediate
  presentation does not acquire payload authority from spelling.
- Keep the exact call-result-to-Trunc-to-final-use edges and exact i64-to-i32
  narrowing. Preserve Cttz/Ctlz contracts; exclude parity and all other
  intrinsic/builtin/direct/indirect calls, ABI/variadic work, pointer/vector/
  aggregate/object families,
  CFG/parameters, inline assembly, and BIR.

## Proof

- Fresh `cmake --build --preset default` passed for the Step-7.19 production
  changes.
- `ctest --test-dir build -R '^frontend_lir_call_type_ref$' --output-on-failure`
  passed 1/1 after focused identity/display/malformed coverage was added.
- Matched focused `test_before.log` / `test_after.log` passed 1/1 before and
  after with delta 0 passed / 0 failed and no new failures.
- The supervisor's matched full regression guard passed 3033/3033 before and
  after, with delta 0 passed / 0 failed and no new failures.
- `git diff --check` passed. The final commit remains supervisor-owned.
