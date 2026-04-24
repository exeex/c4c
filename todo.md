Status: Active
Source Idea Path: ideas/open/lir-hir-to-lir-index-tightening.md
Source Plan Path: plan.md
Current Step ID: Step 1
Current Step Title: Reclassify Top-Level LIR Headers

# Current Packet

## Just Finished

Step 1 from `plan.md` completed the top-level LIR header classification:
`ir.hpp` is documented as the public LIR package index, `call_args.hpp` and
`call_args_ops.hpp` are documented as exported call helper surfaces, and
`operands.hpp` / `types.hpp` are documented as model subheaders re-exported
through `ir.hpp`.

## Suggested Next

Execute Step 2 from `plan.md`: inspect external HIR-to-LIR callers, make
`src/codegen/lir/hir_to_lir.hpp` the public entry header, and keep private
lowering internals behind nested HIR-to-LIR indexes.

## Watchouts

- Step 1 found no need to move `operands.hpp` or `types.hpp`; they are direct
  model subheaders used by `ir.hpp`, with `operands.hpp` also used by
  `call_args.hpp`.
- `call_args.hpp` intentionally stays independent of `LirCallOp`; use
  `call_args_ops.hpp` when code operates on concrete LIR call ops.
- Do not change HIR-to-LIR semantics, LIR semantics, printer output, verifier
  behavior, or testcase expectations.
- Do not introduce one header per `.cpp`.
- Record separate semantic cleanup as a new idea instead of expanding this
  active plan.

## Proof

Passed: `cmake --build --preset default --target c4c_codegen`

Proof log: `test_after.log`
