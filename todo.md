Status: Active
Source Idea Path: ideas/open/251_aarch64_alu_markdown_shard_implementation_redistribution.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Reconcile `alu.md` And Close Shard Ownership

# Current Packet

## Just Finished

Step 4 complete: reconciled `src/backend/mir/aarch64/codegen/alu.md` against
the compiled ALU owner and deleted the markdown shard.

`alu.cpp`/`alu.hpp` now hold the valid current ALU ownership surface: scalar
ALU opcode classification, typed W/X and S/D register-view selection,
prepared-result and prepared-operand record construction from allocation and
storage-plan facts, scalar lowering state for already emitted registers,
integer add/sub/and/or/xor scalar lowering, FP add/sub/mul/div scalar lowering,
and ALU-specific add/sub plus FP printer line construction.

The old shard's durable rebuild warnings were checked before deletion. Its
legacy text-emitter entry points for float negation, integer unary/bit-count
operations, byte-swap, popcount, unsigned power-of-two div/rem reductions,
accumulator fallback div/rem/shift paths, register-direct x0/x1/x2/x3 scratch
conventions, and i128 copy are not live behavior in the current compiled ALU
owner. Those items should remain deferred unless a later semantic-lowering
plan introduces structured MIR support with proof. In particular, do not revive
signed power-of-two div/rem shortcuts, omit signed 32-bit extension semantics
when adding signed 32-bit operations, drop F32 raw-bit zero-extension for a
future fneg route, replace 16-bit bswap with plain `rev w0, w0`, or treat i128
copy as scalar x0-only storage.

## Suggested Next

Step 5 packet: run the plan's final validation/review pass for the ALU shard
redistribution and decide whether lifecycle closure is ready.

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
- Deleted `alu.md` only after recording the remaining legacy-only guidance
  above. Future ALU expansion must use structured MIR records and allocation
  facts rather than reviving the old text-emitter accumulator convention.
- No current blocker remains from the shard reconciliation. The deferred
  unary/bit-count/bswap/popcount/i128-copy and div/rem/shift fallback content
  is not accepted behavior until a later plan adds real semantic lowering and
  targeted proof.

## Proof

Ran supervisor-selected proof:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_'`.

Result: passed. `test_after.log` contains the combined successful build and
backend subset output: 139/139 backend tests passed.
