Status: Active
Source Idea Path: ideas/open/259_aarch64_cast_ops_markdown_shard_implementation_redistribution.md
Source Plan Path: plan.md
Current Step ID: Step 1
Current Step Title: Inventory cast_ops shard and current owners

# Current Packet

## Just Finished

Step 1 of `plan.md` inventoried `src/backend/mir/aarch64/codegen/cast_ops.md`
and the current compiled owners for cast behavior.

Durable shard behavior to preserve:
- scalar casts are classified into no-op/same-size, float-to-int,
  int-to-float, float-width conversion, integer widen, and integer narrow
  routes;
- floating payloads cross the GPR/FP register-bank boundary through `fmov`
  before or after `fcvt`, `scvtf`, `ucvtf`, `fcvtzs`, or `fcvtzu`;
- integer normalization must keep signed `sxtb`/`sxth`/`sxtw` behavior and
  unsigned mask/truncate behavior (`and`, `mov wN, wN`-style zeroing);
- F128 casts stay delegated to the F128 runtime/helper path before ordinary
  scalar cast handling.

Current compiled owners:
- `instruction.hpp` owns cast record types and vocabulary:
  `ScalarCastOperationKind`, `PreparedScalarCastRecordError`, and
  `ScalarCastRecord`.
- `instruction.cpp` owns cast construction and selection details:
  `machine_opcode_from_scalar_cast`, scalar cast selection diagnostics,
  `make_scalar_cast_instruction_record`,
  `make_prepared_scalar_cast_record`, and
  `make_prepared_scalar_cast_instruction_record`.
- `alu.cpp` owns the current scalar-lowering entry point for `bir::CastInst`
  by calling `make_prepared_scalar_cast_instruction_record` inside
  `lower_scalar_instruction`.
- `dispatch.cpp` owns broader cast routing for F128 helper casts and I128
  bitcast transport through `lower_f128_runtime_helper_instruction` and
  `lower_i128_copy_instruction`.
- `machine_printer.cpp` owns cast spelling and printable diagnostics through
  `print_scalar_conversion` and the scalar cast branch inside `print_scalar`.
- `src/backend/CMakeLists.txt` uses explicit AArch64 source registration, so a
  new `cast_ops.cpp` will need to be added there.

Nearby split pattern:
- `alu.cpp`/`alu.hpp`, `comparison.cpp`/`comparison.hpp`, and
  `memory.cpp`/`memory.hpp` keep family-specific record construction,
  lowering entry points, and printer helper declarations in a shard header,
  with `dispatch.cpp` routing to those shard functions.
- The first cast split should mirror that pattern with a minimal
  `cast_ops.hpp` exporting cast-specific construction/lowering helpers while
  still including existing `instruction.hpp`; it should not require an
  instruction hierarchy reshape.

## Suggested Next

First implementation packet: introduce the compiled cast-ops owner shell.

Owned files for that packet:
- `src/backend/mir/aarch64/codegen/cast_ops.cpp`
- `src/backend/mir/aarch64/codegen/cast_ops.hpp`
- `src/backend/CMakeLists.txt`
- `todo.md`

Work:
- add the new header/source pair with namespace, includes, and exported helper
  declarations matching the nearby shard style;
- register `cast_ops.cpp` in `C4C_BACKEND_SOURCES`;
- keep behavior unchanged in this shell packet, moving no cast logic yet unless
  needed for compile-only linkage.

## Watchouts

- Keep this as behavior-preserving cast-ops ownership redistribution.
- Do not expand scalar cast semantics or weaken test expectations.
- Do not touch other AArch64 markdown shards for this active plan.
- Preserve the F128 helper delegation and I128 bitcast transport routes; they
  are cast-shaped routing today but not the first scalar cast shell move.
- The cast printer currently has both conversion spelling and simple integer
  spelling; move those later as spelling helpers, not in the shell packet.

## Proof

Inventory-only packet; build/tests were intentionally not run and no
`test_after.log` was produced.

Proof command for the next implementation slice:

```sh
cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R 'backend_aarch64_.*cast'
```
