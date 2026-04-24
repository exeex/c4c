Status: Active
Source Idea Path: ideas/open/lir-hir-to-lir-index-tightening.md
Source Plan Path: plan.md
Current Step ID: Step 3
Current Step Title: Strengthen The Private HIR-To-LIR Lowering Index

# Current Packet

## Just Finished

Step 3 from `plan.md` strengthened
`src/codegen/lir/hir_to_lir/lowering.hpp` as the private implementation index
for shared HIR-to-LIR lowering. The header now identifies the parent lowering
boundary, separates shared context, module orchestration, constant initializer
support, common helpers, and `StmtEmitter` subdomain entry points, and points
call-only and expression-only details at their nested indexes.

## Suggested Next

Execute Step 4 from `plan.md`: strengthen
`src/codegen/lir/hir_to_lir/call/call.hpp` as the single private index for
call-lowering internals without creating one header per call `.cpp`.

## Watchouts

- `call/call.hpp` currently functions as a thin private directory marker; Step
  4 should make call-lowering helper ownership discoverable there while keeping
  the shared `StmtEmitter` class surface in `lowering.hpp` unless a compile-safe
  class split is explicitly scoped.
- `expr/expr.hpp` is still the matching thin private expression marker for Step
  5; do not mix expression-only cleanup into Step 4.
- Do not change HIR-to-LIR semantics, LIR semantics, printer output, verifier
  behavior, or testcase expectations.
- Do not introduce one header per `.cpp`.
- Record separate semantic cleanup as a new idea instead of expanding this
  active plan.

## Proof

Passed: `cmake --build --preset default --target c4c_codegen`

Proof log: `test_after.log`
