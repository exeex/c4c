Status: Active
Source Idea Path: ideas/open/lir-agent-index-header-hierarchy.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Decide Exported Top-Level LIR Surfaces

# Current Packet

## Just Finished

Step 1: Decide Exported Top-Level LIR Surfaces completed the include-user
inspection and classified the current top-level LIR headers before structural
moves:

- `src/codegen/lir/ir.hpp`: exported surface. It is the core LIR data model and
  is included by backend LIR-to-BIR APIs/tests plus LIR implementation files.
  Retain top-level.
- `src/codegen/lir/hir_to_lir.hpp`: exported surface. It declares the public
  HIR-to-LIR entry point used by `c4cll`, LLVM codegen, and frontend tests.
  Retain top-level, while later steps may move implementation-only helpers out
  of this header if needed.
- `src/codegen/lir/lir_printer.hpp`: exported surface. It declares
  `print_llvm`, used by LLVM/backend integration and frontend tests. Retain
  top-level as the public LIR-to-LLVM-text rendering API.
- `src/codegen/lir/verify.hpp`: candidate for merge into a public LIR-level API.
  Current include users are `verify.cpp` and `lir_printer.cpp`; the public value
  is `verify_module`, while operand/type helper declarations are implementation
  support for verification/printing. Keep top-level only if preserving an
  explicit public verifier surface is desired; otherwise merge `verify_module`
  into the public LIR model/API surface and keep helper details private.
- `src/codegen/lir/call_args.hpp`: exported surface. Its parsing/splitting
  utilities are consumed outside HIR-to-LIR lowering by BIR lowering and AArch64
  MIR emit code. Retain top-level as shared LIR call-text utility API.
- `src/codegen/lir/call_args_ops.hpp`: exported surface. It adds `LirCallOp`
  wrappers over `call_args.hpp` and is included by BIR lowering, the LIR
  printer, and HIR-to-LIR call emission. Retain top-level as the structured
  call-operation utility API.
- `src/codegen/lir/const_init_emitter.hpp`: private lowering state. It is only
  included by `const_init_emitter.cpp` and `hir_to_lir.cpp`, and the class is
  only used during HIR-to-LIR global initializer lowering. Move behind the
  private `src/codegen/lir/hir_to_lir/` lowering index.
- `src/codegen/lir/stmt_emitter.hpp`: private lowering state. It is only
  included by HIR-to-LIR statement-emitter implementation files and
  `hir_to_lir.cpp`, and exposes implementation classes/detail helpers rather
  than cross-component API. Move behind the private
  `src/codegen/lir/hir_to_lir/` lowering index.

## Suggested Next

Start Step 2 by adding the private HIR-to-LIR lowering index and moving
`StmtEmitter`/`ConstInitEmitter` declarations out of top-level headers without
changing lowering behavior.

## Watchouts

Keep `call_args.hpp` and `call_args_ops.hpp` top-level for now because backend
code outside HIR-to-LIR lowering includes their utilities. Treat `verify.hpp`
as a non-lowering merge candidate, not part of the private lowering move.

## Proof

No build proof was run; the delegated packet was inspection-only and changed
only `todo.md`. No `test_after.log` was produced.
