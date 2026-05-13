# Current Packet

Status: Active
Source Idea Path: ideas/open/204_aarch64_prepared_module_mir_boundary.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Inspect Prepared/AArch64 Boundary Surfaces

## Just Finished

Step 1 inspected the prepared-module and backend boundary surfaces for
`plan.md` Step 1. `PreparedBirModule` already carries the target profile,
prepared name tables, control-flow, value locations, stack/frame/dynamic-stack
plans, addressing, liveness, regalloc, call plans, storage plans, invariants,
and completed phases needed for an AArch64 MIR handoff gate. The x86 precedent
is a thin prepared-module API (`src/backend/mir/x86/api/api.hpp`), ABI/profile
resolver (`src/backend/mir/x86/abi/abi.hpp`), and module consumer
(`src/backend/mir/x86/module/module.hpp`); AArch64 currently has only
text-oriented `codegen/emit.hpp` entrypoints and no prepared MIR consumer.

## Suggested Next

Implement Step 2 as a compile-proven AArch64 prepared-module handoff gate.
Recommended files:

- `src/backend/mir/aarch64/abi/abi.hpp`
- `src/backend/mir/aarch64/abi/abi.cpp`
- `src/backend/mir/aarch64/module/module.hpp`
- `src/backend/mir/aarch64/module/module.cpp`
- `src/backend/mir/aarch64/api/api.hpp`
- `src/backend/mir/aarch64/api/api.cpp`
- `tests/backend/backend_aarch64_prepared_handoff_gate_test.cpp`
- `src/backend/CMakeLists.txt`
- `tests/backend/CMakeLists.txt`

The Step 2 API should expose a prepared-module entry such as
`c4c::backend::aarch64::api::build_prepared_module(const PreparedBirModule&)`
or an equivalent target-local builder. The gate should resolve
`PreparedBirModule::target_profile`, reject non-`TargetArch::Aarch64`, require
`BackendAbiKind::Aapcs64`, and return/throw a typed backend handoff failure
before constructing any instruction-selection or assembly-text records. The
first focused test target should be
`backend_aarch64_prepared_handoff_gate_test`, registered as CTest
`backend_aarch64_prepared_handoff_gate`.

Exact Step 2 proof command:

```sh
cmake --build --preset default --target backend_aarch64_prepared_handoff_gate_test > test_after.log 2>&1 && ctest --test-dir build -R '^backend_aarch64_prepared_handoff_gate$' --output-on-failure >> test_after.log 2>&1
```

## Watchouts

- Do not edit `ideas/open/204_aarch64_prepared_module_mir_boundary.md` unless
  source intent itself proves wrong.
- Do not add instruction selection, assembly text emission, assembler/object,
  linker, or executable production.
- Do not route Step 2 through existing AArch64 `codegen/emit.hpp`; it still
  exposes raw BIR/LIR/text paths and is outside the prepared MIR handoff.
- Do not recover semantic facts from rendered names, printed BIR, legacy LIR
  text, assembly strings, parser operands, or markdown examples.
- Do not weaken, skip, or reclassify tests to claim boundary progress.
- `PreparedMemoryAccess` still lacks explicit volatility/address-space fields;
  that is not a Step 2 gate blocker, but do not start memory lowering from it.
- Preserve unrelated dirty files and transient `review/` artifacts.

## Proof

Delegated Step 1 proof command:

```sh
cmake --build --preset default > test_after.log 2>&1
```

Proof log: `test_after.log`.
