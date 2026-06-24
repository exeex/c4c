Status: Active
Source Idea Path: ideas/open/340_rv64_standard_insn_inline_asm_stage1.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Map Existing Inline-Asm And RV64 Object Route

# Current Packet

## Just Finished

Step 1 mapped the existing inline-asm and RV64 object surfaces for standard
`.insn` Stage 1.

- Frontend/HIR to LIR carrier: `src/codegen/lir/hir_to_lir/stmt.cpp`
  (`StmtEmitter::emit_non_control_flow_stmt(const InlineAsmStmt&)`) renders
  outputs, scalar read-write outputs as `"=r,0"`, inputs, clobbers, side effects,
  and `args_str` into `lir::LirInlineAsmOp`; `src/codegen/lir/ir.hpp`
  (`LirInlineAsmOp`) is the current LIR payload.
- LIR to BIR metadata: `src/backend/bir/lir_to_bir/calling.cpp`
  (`make_inline_asm_metadata`, `lower_inline_asm_call`) lowers the payload into
  `bir::CallInst{callee="llvm.inline_asm", inline_asm=...}` and classifies
  `r`, `=r`, `i/I`, `m`, `p`, decimal tied operands, and clobbers. Stage 1
  `+r` currently reaches BIR as the scalar-output/readwrite pair `"=r,0"`, not
  as a retained `+r` token.
- BIR carrier structs: `src/backend/bir/bir.hpp` (`InlineAsmMetadata`,
  `InlineAsmOperandMetadata`, `InlineAsmOperandKind`, `CallInst::inline_asm`)
  already preserve template text, constraints, args text fallback, side effects,
  clobbers, operand order, output indexes, arg indexes, and tied output indexes.
- Prealloc carrier: `src/backend/prealloc/prealloc.cpp`
  (`BirPreAlloc::publish_contract_plans`) calls
  `src/backend/prealloc/inline_asm.cpp` (`populate_inline_asm_carriers`,
  `build_inline_asm_carrier`, `make_prepared_inline_asm_operand`,
  `validate_inline_asm_carrier`) to attach prepared homes, immediate values,
  missing facts, and `PreparedInlineAsmCarriers`; `src/backend/prealloc/module.hpp`
  (`PreparedBirModule::inline_asm_carriers`,
  `find_prepared_inline_asm_carriers`) is the lookup surface.
- RV64 prealloc prerequisite: `src/backend/prealloc/inline_asm.cpp`
  (`inline_asm_register_identity`) has AArch64 register identity support only;
  `TargetArch::Riscv64` currently returns `nullopt`. RV64 scalar `r`, `=r`,
  and tied `"0"` support needs an RV64 GPR identity mapper here so tied-home
  validation and later substitution can prove concrete register homes.
- Existing RV64 asm substitution helpers: `src/backend/mir/riscv/codegen/asm_emitter.cpp`
  and `src/backend/mir/riscv/codegen/inline_asm.cpp`
  (`classify_rv_constraint`, `substitute_riscv_asm_operands`,
  `format_riscv_operand`) are standalone helper translations, but they are not
  wired into `PreparedBirModule` object emission. They also substitute textual
  operands, not `.insn` object bytes.
- RV64 object route: `src/backend/mir/riscv/codegen/object_emission.cpp`
  (`prepared_function_to_object_function`) currently accepts single-block
  prepared functions and emits fragments for binary ops, local load/store,
  direct calls, rematerialized immediates, and returns. Inline-asm calls route
  through `fragment_for_prepared_call`, so a `llvm.inline_asm` `.insn` call
  currently has no target fragment path.
- RV64 byte encoder/object surfaces to extend: `src/backend/mir/riscv/codegen/object_emission.cpp`
  has reusable `encode_r_type`, `append_le32`, `rv64_register_number`,
  `gpr_register_number_for_home`, and `build_rv64_text_object_module`.
  Stage 1 should add a target-owned `.insn r` parser/classifier and a fragment
  builder used from `prepared_function_to_object_function`, rather than raw
  assembler delegation or template string passthrough.
- Narrow byte-level proof harness: `tests/backend/mir/backend_riscv_object_emission_test.cpp`
  already checks `.text` bytes from `build_rv64_prepared_text_object_module`
  with `read_u32`; this is the best unit-level hook for exact `.insn` bytes.
  CLI/object-route coverage belongs in `tests/backend/CMakeLists.txt`
  via `c4c_add_backend_codegen_object_test` and
  `tests/backend/cmake/run_backend_codegen_object_case.cmake`, which invokes
  `c4cll --codegen obj --target riscv64-linux-gnu` and checks ELF shape.

## Suggested Next

Delegate Step 2 to preserve/classify Stage 1 scalar constraints for RV64:
add RV64 GPR register identity support in `src/backend/prealloc/inline_asm.cpp`,
confirm `r`, `=r`, and tied `"0"` carriers are complete under
`TargetArch::Riscv64`, and add focused BIR/prealloc tests before object encoding.

## Watchouts

- Keep the active route limited to standard RV64 `.insn` scalar constraints.
- Do not absorb vector constraints, EV `.insn.d`, named operands, or consteval
  asm string work into this child.
- Treat external assembler proof, testcase-shaped string matching, or
  expectation downgrades as route drift.
- Stage 1 `.insn` object support should be target-owned parsing/encoding in the
  RV64 object route. The existing RV64 textual substitution helpers are useful
  reference code, but wiring them alone would not prove object bytes.
- The existing generic BIR classifier does not retain a literal `+r`; scalar
  read-write output is normalized to output `=r` plus tied input `0`.
- RV64 object emission currently rejects unsupported call shapes by returning
  `nullopt`; adding `.insn` support likely means handling `call.inline_asm`
  before `fragment_for_prepared_call`.

## Proof

`( cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_' ) > test_after.log 2>&1`

Ran the exact delegated proof command. It exited nonzero with the known baseline
failure still present: 315/316 backend tests passed, and the only failing test
was `backend_codegen_route_riscv64_pointer_typed_select_publication` due to the
existing forbidden snippet `mv t0, t0`. Proof log path: `test_after.log`.
