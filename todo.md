# Current Packet

Status: Active
Source Idea Path: ideas/open/741_lir_structured_operand_and_terminator_identity_decomposition.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Implement the narrowest generic carrier

## Just Finished

- Plan Step 5 packet 5 implements CC-RET-1 for scalar integer returns.
- `LirRet` now owns one optional `LirOperand` and one `LirTypeRef`; the legacy
  field spellings remain only as aggregate-initializer compatibility and no
  parallel raw semantic mirror exists.
- Ordinary integer literals, synthesized integer zero, and neighboring scalar
  global loads populate native immediate or function-local SSA authority.
  Representation-changing coercions remain raw compatibility; void expression
  returns emit their side effect and a valueless structured-void terminator.
- The verifier enforces void/value shape, type parity, supported authority,
  immediate range, and current-function SSA ownership. The printer preserves
  presentation after structured verification.

## Suggested Next

- Execute Step 6's final authority-matrix audit and handoff to idea 734. Do not
  add new-BIR receipt while proving and documenting the resumable boundary.

## Watchouts

- Raw `LirRet` construction remains compatible for unowned producer families,
  but authoritative scalar returns accept only integer immediate or
  current-function `LirValueId` authority. `LinkNameId`, malformed parity,
  out-of-range immediates, unknown IDs, and cross-function IDs reject.
- Equal LLVM value types are the no-instruction early-return path in `coerce`;
  the producer preserves authority only there. A narrowing-cast test proves
  emitted coercions do not inherit the pre-coercion authority.
- New-BIR still receives only void returns. Its structured boundary rejects any
  value or non-void type as `InvalidVoidReturn` before display interpretation;
  scalar return receipt remains explicitly out of scope.

## Proof

- `cmake --build --preset default` passed.
- `ctest --test-dir build -R '^frontend_lir_call_type_ref$'
  --output-on-failure` passed 1/1 with literal/load/synthesized/void producer
  inspection, misleading displays, coercion coverage, raw compatibility, and
  the complete malformed return matrix.
- `backend_lir_to_bir_interface` passed 1/1 and proves an authoritative scalar
  return remains `InvalidVoidReturn` despite misleading presentation.
- The CLI has no `--dump-lir` option; the frontend test directly inspects LIR
  native facts. Focused `--codegen llvm` retained `ret i32 7`. All four focused
  `--dump-bir` probes retained their Step 3 boundaries: store/load/GEP are
  `UnsupportedOrdinaryInstruction`; scalar return is `InvalidVoidReturn`.
- Exact full proof `ctest --test-dir build -j --output-on-failure >
  test_after.log` passed 3033/3033, matching `test_before.log`. `git diff
  --check` passed.
