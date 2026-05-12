Status: Active
Source Idea Path: ideas/open/190_lir_call_argument_structured_payload_boundary.md
Source Plan Path: plan.md
Current Step ID: Step 2
Current Step Title: Define The Structured Argument Carrier

# Current Packet

## Just Finished

Completed `plan.md` Step 2 by adding a narrow structured call argument carrier
at the `LirCallOp` boundary.

Implemented `LirCallArg` in `src/codegen/lir/ir.hpp` with argument type text,
operand text as `LirOperand`, and an optional-by-content `LirTypeRef` copied
from `OwnedLirTypedCallArg` when available. Added
`LirCallOp::structured_args`, default-empty so direct aggregate/manual
`LirCallOp` construction remains no-carrier raw compatibility.

Updated `make_lir_call_op_with_return_type_ref` in
`src/codegen/lir/call_args_ops.hpp` to populate `structured_args` from
`OwnedLirTypedCallArg` while leaving `callee_type_suffix` and `args_str` as the
rendered call spelling and preserving existing `arg_type_refs` mirror behavior.

Extended `tests/frontend/frontend_lir_call_type_ref_test.cpp` to prove generated
direct, byval, variadic, no-prototype, and indirect calls carry structured
argument facts, and that raw direct `LirCallOp` construction still has an empty
carrier.

## Suggested Next

Execute `plan.md` Step 3 by routing metadata-rich LIR-to-BIR call lowering
through `LirCallOp::structured_args` for argument operands and argument type
text when the carrier is present, leaving the text parser as the explicit
no-carrier compatibility path.

## Watchouts

- Do not continue the idea 188 closure gate until ideas 190, 191, and 194 are
  closed or the closure-gate source idea is explicitly narrowed.
- BIR lowering has not been changed yet; current semantic consumers still parse
  `args_str`.
- `structured_args[].type_ref` is only populated when the existing
  `OwnedLirTypedCallArg` carried one. Some scalar and variadic-tail arguments
  currently have structured type/operand text but an empty `LirTypeRef`.
- `arg_type_refs` is still the old mirror-only vector and may remain empty when
  the call is not mirrorable against rendered signature text; Step 3 should
  consume `structured_args` for the full generated argument list.
- The printer still uses `callee_type_suffix` and `args_str`, so stale rendered
  text can still affect printed LLVM text until a later semantic proof separates
  backend authority from spelling.

## Proof

Ran:

`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(frontend_lir_call_type_ref|backend_lir_to_bir_notes)$' > test_after.log`

Result: passed, `2/2` tests green. Proof log: `test_after.log`.
