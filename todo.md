Status: Active
Source Idea Path: ideas/open/lir-agent-index-header-hierarchy.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Reassess Call Argument Surfaces

# Current Packet

## Just Finished

Step 4: Reassess Call Argument Surfaces completed the inspection-only
classification for `call_args.hpp` and `call_args_ops.hpp`:

- Keep `src/codegen/lir/call_args.hpp` exported as a top-level LIR API. It owns
  syntax-level call parsing, formatting, operand scanning, and typed argument
  helpers used outside HIR-to-LIR.
- Keep `src/codegen/lir/call_args_ops.hpp` exported as a top-level LIR API. It
  adapts the same helpers to `LirCallOp` and is used by printer and BIR
  lowering, not just by HIR-to-LIR emitters.
- Relevant cross-component users found:
  `src/codegen/lir/lir_printer.cpp`,
  `src/backend/bir/lir_to_bir/lowering.hpp`,
  `src/backend/bir/lir_to_bir/calling.cpp`,
  `src/backend/bir/lir_to_bir/memory/coordinator.cpp`, and
  `src/backend/mir/aarch64/codegen/emit.cpp`.
- HIR-to-LIR users remain implementation consumers:
  `src/codegen/lir/hir_to_lir/lowering.hpp`,
  `src/codegen/lir/hir_to_lir/hir_to_lir.cpp`, and the call/vaarg/expression
  statement emitters.
- No implementation-only call-lowering state was identified in these headers,
  so a private `src/codegen/lir/hir_to_lir/call/` subdomain is not warranted
  now.

## Suggested Next

Start Step 5: Final Structural Validation and Cleanup. Confirm the final
top-level LIR header set, including whether `verify.hpp` is an intentionally
retained exported validation surface or should be cleaned up as part of the
final structure pass.

## Watchouts

Do not move `call_args.hpp` or `call_args_ops.hpp` under `hir_to_lir/` without
first splitting out any truly private lowering-only helpers. The current
top-level headers are shared LIR text/call semantics used by backend lowering
and printing.

## Proof

No build proof required by packet; this was inspection-only and changed only
`todo.md`. Existing `test_after.log` was not touched.
