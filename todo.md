Status: Active
Source Idea Path: ideas/open/lir-hir-to-lir-index-tightening.md
Source Plan Path: plan.md
Current Step ID: Step 2
Current Step Title: Tighten The Public HIR-To-LIR Entry Header

# Current Packet

## Just Finished

Step 2 from `plan.md` tightened `src/codegen/lir/hir_to_lir.hpp` into the
public HIR-to-LIR entry header. External callers now get the LIR return model
through `ir.hpp`, while module/function skeleton helpers and lowering
orchestration declarations live behind the private
`src/codegen/lir/hir_to_lir/lowering.hpp` implementation index.

## Suggested Next

Execute Step 3 from `plan.md`: strengthen
`src/codegen/lir/hir_to_lir/lowering.hpp` as the private implementation index
for shared HIR-to-LIR lowering context and common helper declarations.

## Watchouts

- External users under `src/` already include `hir_to_lir.hpp` rather than
  nested HIR-to-LIR private indexes.
- `hir_to_lir.hpp` intentionally exposes only `LowerOptions` and `lower(...)`;
  keep statement, expression, call, and module orchestration helpers behind
  nested private headers.
- `lowering.hpp` now owns declarations moved out of the public header; Step 3
  should refine that private index without re-exporting it publicly.
- Do not change HIR-to-LIR semantics, LIR semantics, printer output, verifier
  behavior, or testcase expectations.
- Do not introduce one header per `.cpp`.
- Record separate semantic cleanup as a new idea instead of expanding this
  active plan.

## Proof

Passed: `cmake --build --preset default --target c4c_codegen`

Proof log: `test_after.log`
