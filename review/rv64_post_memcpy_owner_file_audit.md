# RV64 Post-Memcpy Owner-File Audit

Date: 2026-04-12
Active Plan: `plan.md`
Source Idea: `ideas/open/45_rv64_backend_codegen_bringup_from_x86_backend_framework.md`

## Audit Scope

- reviewed the live shared RV64 surface in
  `src/backend/riscv/codegen/riscv_codegen.hpp`
- reviewed the just-completed `memory.cpp` helper lane in
  `src/backend/riscv/codegen/memory.cpp`
- reviewed parked translated owner files with visible next-packet candidates:
  `src/backend/riscv/codegen/alu.cpp`,
  `src/backend/riscv/codegen/globals.cpp`,
  `src/backend/riscv/codegen/calls.cpp`,
  `src/backend/riscv/codegen/float_ops.cpp`,
  `src/backend/riscv/codegen/comparison.cpp`,
  `src/backend/riscv/codegen/cast_ops.cpp`, and
  `src/backend/riscv/codegen/variadic.cpp`

## Findings

- the remaining visible `memory.cpp` bodies are still broader than the helper
  lane that just landed because they depend on default load/store delegation,
  typed indirect addressing, wider GEP ownership, and broader shared RV64
  state
- `globals.cpp` is smaller than calls or broader memory work, but the live C++
  surface still lacks the state and result-store helpers it expects:
  `state.needs_got(...)` and `store_t0_to(...)`
- `calls.cpp` still carries a wide ABI surface and is not a credible bounded
  next packet from the current translated seam
- `float_ops.cpp`, `comparison.cpp`, `cast_ops.cpp`, and `variadic.cpp`
  currently preserve local helper logic, but their owner bodies still depend
  on broader shared `RiscvCodegen` / operand / destination surfaces
- `alu.cpp` exposes one isolated owner seam that fits the live surface now:
  `emit_float_neg_impl(IrType)`, `emit_int_neg_impl(IrType)`, and
  `emit_int_not_impl(IrType)` only need `IrType` plus `RiscvCodegen::state.emit`
  and do not require new GOT tracking, result-store helpers, call ABI state,
  or broader memory ownership

## Route Decision

The next bounded RV64 packet should move out of `memory.cpp` and into the
translated unary-helper seam in `src/backend/riscv/codegen/alu.cpp`.

The bounded packet should:

- wire `emit_float_neg_impl(...)`, `emit_int_neg_impl(...)`, and
  `emit_int_not_impl(...)` into the shared `riscv_codegen.hpp` surface
- add only the minimum `CMakeLists.txt` entries needed to compile
  `src/backend/riscv/codegen/alu.cpp`
- add focused shared-util coverage for the bounded unary emission text

Keep these families parked unless the ALU packet proves otherwise:

- broader `memory.cpp` owner work
- `globals.cpp` owner work
- `calls.cpp` ABI work
- broader float/comparison/cast/variadic owner bodies

## Rationale

This route keeps the lane integration-first and bounded. It advances a real
translated owner file, but it does so through a helper-sized seam that matches
the existing shared header instead of forcing a larger state expansion just to
keep momentum after the completed memcpy loop.
