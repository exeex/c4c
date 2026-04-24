Status: Active
Source Idea Path: ideas/open/lir-hir-to-lir-index-tightening.md
Source Plan Path: plan.md
Current Step ID: Step 4
Current Step Title: Strengthen The Private Call-Lowering Index

# Current Packet

## Just Finished

Step 4 from `plan.md` strengthened
`src/codegen/lir/hir_to_lir/call/call.hpp` as the single private implementation
index for call lowering. The index now owns the call-target data model,
argument-preparation state, builtin-call helper declarations, direct call
emission helpers, and `va_arg` helper surface while preserving standalone
`#include "call.hpp"` usage from call-lowering implementation files.

## Suggested Next

Execute Step 5 from `plan.md`: strengthen
`src/codegen/lir/hir_to_lir/expr/expr.hpp` as the single private index for
expression-lowering internals without creating one header per expression `.cpp`.

## Watchouts

- `call/call.hpp` uses scoped include modes because call declarations are
  private `StmtEmitter` members that must still be declared inside the class in
  `lowering.hpp`; preserve that boundary unless a larger class split is
  explicitly scoped.
- `expr/expr.hpp` is still the matching thin private expression marker for Step
  5; do not move call-only declarations back into the expression section.
- Do not change HIR-to-LIR semantics, LIR semantics, printer output, verifier
  behavior, or testcase expectations.
- Do not introduce one header per `.cpp`.
- Record separate semantic cleanup as a new idea instead of expanding this
  active plan.

## Proof

Passed: `cmake --build --preset default --target c4c_codegen`

Proof log: `test_after.log`
