Status: Active
Source Idea Path: ideas/open/251_aarch64_alu_markdown_shard_implementation_redistribution.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Inspect ALU Ownership Surfaces

# Current Packet

## Just Finished

Step 1 complete: inspected the ALU legacy shard and current compiled AArch64
ownership surfaces without implementation edits.

Concrete extraction targets for the first implementation packet:

- Move scalar ALU record construction out of broad `instruction.cpp` into
  `alu.cpp`/`alu.hpp`: `is_scalar_alu_integer_opcode`,
  `is_scalar_alu_floating_opcode`, `is_scalar_alu_floating_type`,
  `scalar_alu_operation_from_binary_opcode`,
  `make_prepared_scalar_alu_record`,
  `make_prepared_scalar_alu_instruction_record`, and
  `make_scalar_alu_instruction_record`.
- Move the ALU operand-preparation helper cluster that exists only to build
  scalar ALU records: `make_scalar_immediate_operand` and
  `make_prepared_scalar_operand`. Keep shared lookup helpers only if the next
  packet can avoid duplicating `find_named_value_home` and
  `find_storage_plan_value`; otherwise expose a narrow ALU-specific wrapper
  from `alu.hpp`.
- Keep `alu.cpp`'s existing block-level owner role for
  `lower_scalar_instruction`, `BlockScalarLoweringState`,
  emitted-register tracking, return-chain targeting, and scalar fallback
  operand preparation. The next packet should route prepared and fallback ALU
  construction through ALU-owned declarations instead of adding new bodies to
  `dispatch.cpp`.
- Leave i128 pair, shift, compare, runtime-helper boundary, and transport
  records outside this ALU packet. They are adjacent integer machinery, but the
  current `alu.md` i128 durable behavior is only two-register copy/transport;
  do not pull i128 arithmetic ownership into the scalar ALU extraction.
- Printer-side ALU spelling remains a later packet: `machine_printer.cpp`
  currently owns `floating_alu_mnemonic` and the scalar add/sub/floating
  spelling inside `print_scalar`. Record this as a Step 3 target, not the first
  implementation move.

## Suggested Next

First code-changing packet for Step 2: move scalar ALU record construction and
opcode classification declarations/bodies from `instruction.cpp`/`instruction.hpp`
into `alu.cpp`/`alu.hpp`, keeping `instruction.cpp` as the family-neutral
`InstructionRecord` assembler and preserving the existing selected-node
behavior.

## Watchouts

- Do not redistribute other AArch64 markdown shards in this route.
- Preserve ALU behavior; do not downgrade tests or weaken supported-path
  contracts to claim progress.
- Keep broad owners family-neutral: `instruction.cpp` may retain
  `InstructionFamily`, `MachineOpcode`, `MachinePrinterMnemonicKind`, shared
  result/operand record structs, generic `make_scalar_instruction`, machine
  opcode-to-family machinery, compare/cast/memory/i128/f128/atomic/call/return
  construction, and shared diagnostics.
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

Inspection-only lifecycle packet; no build/tests run and no `test_after.log`
written.

Supervisor-selected narrow proof command for future implementation packets:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_'`.
