# Current Packet

Status: Active
Source Idea Path: ideas/open/744_lir_remaining_ordinary_value_identity_publication.md
Source Plan Path: plan.md
Current Step ID: 7.20
Current Step Title: Publish builtin-parity result/use authority

## Just Finished

- Accepted Plan Step 7.19 for only PI's i32/i64 builtin-popcount `llvm.ctpop`
  call, optional i64-to-i32 Trunc, and one later ordinary i32 use.
- The matched focused and full regression proofs passed, and commit `8929002bd`
  records the coherent Step-7.19 slice.
- Step 7 is not complete: the checked matrix still names parity and other
  production ordinary rows as unclaimed rather than exact separately owned
  blockers.

## Suggested Next

- Executor: complete Plan Step 7.20 for only PI's i32/i64
  `emit_builtin_parity_call` route. Publish the native Ctpop call result into
  the scalar integer And-with-one result, preserve any required exact
  i64-to-i32 Trunc, and carry the final authoritative i32 ID into one later
  ordinary use.

## Watchouts

- Reuse the accepted Ctpop contract: native `LirIntrinsicKind::Ctpop`, one
  module-owned callee `LinkNameId`, an exact fixed nonvariadic
  one-integer-parameter signature, and no `zero_count_behavior`. Do not infer
  semantics or identity from intrinsic, result, operand, or temporary text.
- Preserve arg0's honest compatibility boundary: available native authority
  must be a valid current-function SSA ID, while monostate SSA or Immediate
  presentation must not acquire payload authority from spelling.
- Allocate the parity And result through `fresh_value`, require native integer
  And opcode/type authority, preserve the exact Ctpop result as its lhs and an
  exactly representable integer immediate one as its rhs, then preserve the
  exact result through only the route-required i64-to-i32 Trunc and final i32
  use.
- Reachable verification must reject invalid/duplicate/unknown/cross-function
  result or use IDs; wrong intrinsic kind, callee, signature, counts, opcode,
  type, immediate payload, cast kind/endpoints, or chain endpoints; any
  zero-count behavior; and invented authority for compatible literal input.
  Misleading displays with unchanged native facts must pass.
- Exclude popcount's already-accepted direct result route, all other
  intrinsic/builtin calls and binary/cast producers, direct/indirect/variadic/
  ABI call expansion, pointer/vector/complex/aggregate/object families,
  CFG/parameters, inline assembly, BIR receipt, and all idea-741 contracts.
- This packet is carrier-ready only on the existing Ctpop, scalar integer
  BinOp, optional Trunc, and generic ownership seams. If exact parity
  publication requires a new semantic carrier or a text-derived bridge, stop
  and return that exact blocker; do not widen Step 7.20.

## Proof

- Fresh `cmake --build --preset default`.
- Focused `ctest --test-dir build -R '^frontend_lir_call_type_ref$' --output-on-failure`
  covering i32/i64 parity identity, misleading-display positives, and every
  malformed native fact and chain edge owned above.
- Preserve nearby accepted Cttz/Ctlz/Ctpop and scalar integer BinOp/Trunc
  coverage unchanged; the supervisor owns matched regression logs and any
  broader/full proof.
- Run `git diff --check` before handoff.
