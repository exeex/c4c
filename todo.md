# Current Packet

Status: Active
Source Idea Path: ideas/open/819_lir_scalar_binary_lhs_parameter_authority.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Discover the native scalar binary-LHS authority producer

## Just Finished

- Plan Step 1 discovery completed with a fail-closed result: no complete native
  scalar `LirBinOp.lhs` parameter map exists. `emit_binary_rval_operand`
  (`src/codegen/lir/hir_to_lir/expr/binary.cpp:162`) preserves an existing
  authoritative operand only when it already has authority, then publishes it
  as `LirBinOp.lhs` (`:560-564`); its scalar role is therefore explicitly lhs,
  but it creates no parameter authority. The current-function producer
  `init_fn_ctx` (`hir_to_lir.cpp:1395-1424`) gives every parameter only the
  forbidden spelling-only `ctx.param_slots` entry; it allocates a native value
  and `LirCurrentFunctionBodyParameterDefinition` only for pointer parameters.
  `populate_lir_function_params`/`populate_signature_type_refs`
  (`hir_to_lir.cpp:325-382`) provide logical position/type and signature mirror
  data, while `LirFunction.link_name_id` provides owner, but neither is joined
  to a scalar body value. The schema confirms the gap:
  `LirCurrentFunctionBodyParameterDefinition` has value, parameter_index, type,
  owner, and ABI, but its only admitted ABI is `DirectPointer`
  (`ir.hpp:884-902`); `LirBinOp` has result/opcode/type/lhs/rhs and no
  parameter binding or role tuple (`ir.hpp:538-544`). The verifier admits a
  generic authoritative binary lhs and merely requires its `LirValueId` to be
  a known current-function definition (`verify.cpp:1690-1702`, `:3041-3055`);
  `verify_native_body_parameter_definitions` explicitly requires pointer type
  and `DirectPointer` ABI (`:2121-2146`). Thus the first missing fact is a
  native scalar current-function parameter `LirValueId` joined to its position,
  scalar type, owner, ABI class, and explicit binary-lhs role. No
  presentation-derived substitute was used.
- Fail-closed boundary for the eventual selected tuple: reject absent/invalid
  value, duplicate value or parameter index, foreign/non-unique owner, missing
  or non-lhs role, out-of-range position, scalar type/mirror mismatch,
  unsupported or incoherent ABI, non-scalar form, an lhs that does not mirror
  the selected value, and every `param_slots`/rendered-name/`raw`-derived form.

## Suggested Next

- One minimal publishing packet: extend the native body-parameter schema and
  producer for exactly one plain fixed scalar current-function parameter
  `LirValueId` plus position/type/owner/direct-scalar ABI and `LirBinOp.lhs`
  binding, then make the verifier enforce the enumerated boundary.

## Watchouts

- `param_slots`, rendered names/signatures, `LirOperand::raw`, and
  `preserve_exact_binary_operand` string matching are forbidden authority.
- Do not touch Raw-BIR/importer/dispatcher/receiver work; do not reuse closed
  795/817 or byval-memcpy forms.
- Existing `native_body_parameter_definitions` is intentionally pointer-only;
  do not relabel `DirectPointer` or infer a scalar value from its absence.

## Proof

- Passed: `cmake --build --preset default && ctest --test-dir build -j
  --output-on-failure -R '^backend_' 2>&1 | tee test_after.log` (6/6 backend
  tests); log: `test_after.log`. The delegated build-plus-subset proof is
  sufficient for this trace-only packet.
