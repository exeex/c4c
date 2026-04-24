Status: Active
Source Idea Path: ideas/open/lir-hir-to-lir-index-tightening.md
Source Plan Path: plan.md
Current Step ID: Step 5
Current Step Title: Strengthen The Private Expression-Lowering Index

# Current Packet

## Just Finished

Step 5 from `plan.md` strengthened
`src/codegen/lir/hir_to_lir/expr/expr.hpp` as the single private implementation
index for expression lowering. The index now owns the expression member
declaration surface for type resolution, rval coordination, binary helpers, and
miscellaneous payload emitters while preserving standalone `#include "expr.hpp"`
usage from expression-lowering implementation files.

## Suggested Next

Execute Step 6 from `plan.md`: run structural validation and close-readiness
review for the LIR HIR-to-LIR index-tightening plan.

## Watchouts

- `call/call.hpp` and `expr/expr.hpp` both use scoped include modes because
  private `StmtEmitter` member declarations must still be declared inside the
  class in `lowering.hpp`; preserve that boundary unless a larger class split is
  explicitly scoped.
- Step 5 intentionally moved declarations behind the expression index only; it
  did not change expression lowering semantics or introduce per-expression
  headers.
- Do not change HIR-to-LIR semantics, LIR semantics, printer output, verifier
  behavior, or testcase expectations.
- Do not introduce one header per `.cpp`.
- Record separate semantic cleanup as a new idea instead of expanding this
  active plan.

## Proof

Passed: `cmake --build --preset default --target c4c_codegen`

Proof log: `test_after.log`
