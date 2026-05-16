Status: Active
Source Idea Path: ideas/open/251_aarch64_alu_markdown_shard_implementation_redistribution.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Move ALU Spelling And Printer-Side Bodies

# Current Packet

## Just Finished

Step 3 complete: moved ALU-specific scalar printer spelling and line-building
out of `machine_printer.cpp` into `alu.cpp`/`alu.hpp` without changing printed
assembly.

`alu.cpp` now owns the scalar FP mnemonic spellings, add/sub immediate
spellings, and scalar ALU print line construction through
`make_scalar_alu_print_lines`. `machine_printer.cpp` keeps the family-neutral
scalar dispatch, cast printing, result wrapping, and unsupported diagnostic
prefixing.

## Suggested Next

Step 4 packet: reconcile remaining `alu.md` durable behavior/risk notes against
the compiled ALU owner before any shard deletion or broader redistribution.

## Watchouts

- Do not redistribute other AArch64 markdown shards in this route.
- Preserve ALU behavior; do not downgrade tests or weaken supported-path
  contracts to claim progress.
- Keep broad owners family-neutral: `instruction.cpp` may retain
  `InstructionFamily`, `MachineOpcode`, `MachinePrinterMnemonicKind`, shared
  result/operand record structs, generic `make_scalar_instruction`, machine
  opcode-to-family machinery, compare/cast/memory/i128/f128/atomic/call/return
  construction, and shared diagnostics.
- Existing tests include `instruction.hpp` for ALU helper declarations, so that
  header currently keeps compatibility declarations even though the bodies and
  primary owner declarations live in `alu.cpp`/`alu.hpp`.
- Keep `dispatch.cpp` as a router. It may continue to construct
  `BlockScalarLoweringState`, call `lower_scalar_instruction`, and sequence
  intrinsic, i128, f128, memory, call, and inline-asm lowering around scalar
  lowering, but it should not gain ALU operation bodies.
- Keep `machine_printer.cpp` family-neutral for generic register spelling,
  immediate formatting, cast printing, i128/f128 printing, intrinsic printing,
  and final payload dispatch. Only move ALU-specific scalar mnemonic/spelling
  helpers in the later printer packet.
- Do not delete `alu.md` until its durable behavior and risks are reconciled
  into compiled ALU ownership.
- Preserve the shard risks when moving code: no signed power-of-two div/rem
  reduction, do not lose signed 32-bit `sxtw` or unsigned zero-extension
  behavior, keep right-operand destination-register conflicts explicit if that
  register-direct path is rebuilt, keep byte-swap/F32-neg/popcount/i128-copy
  notes for later reconciliation, and do not expand supported opcodes while
  claiming a behavior-preserving ownership move.

## Proof

Ran supervisor-selected proof:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_'`.

Result: passed. `test_after.log` contains the combined successful build and
backend subset output.
