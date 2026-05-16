Status: Active
Source Idea Path: ideas/open/251_aarch64_alu_markdown_shard_implementation_redistribution.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Prove Behavior Preservation And Ownership Boundaries

# Current Packet

## Just Finished

Step 5 complete: ran the final validation pass for the AArch64 ALU shard
redistribution after the compiled owner moves and `alu.md` deletion.

Final ownership state:

- `alu.cpp`/`alu.hpp` own scalar ALU opcode classification, typed W/X and S/D
  register-view selection, prepared-result and prepared-operand record
  construction from allocation and storage-plan facts, scalar lowering state
  for already emitted registers, integer add/sub/and/or/xor scalar lowering,
  FP add/sub/mul/div scalar lowering, and ALU-specific add/sub plus FP printer
  line construction.
- `instruction.cpp` keeps family-neutral record assembly and other non-ALU
  record families.
- `machine_printer.cpp` routes scalar ALU spelling to the ALU owner while
  keeping family-neutral dispatch, cast printing, result wrapping, and
  unsupported diagnostic prefixing.
- `src/backend/mir/aarch64/codegen/alu.md` was deleted after reconciliation.

Reviewer check: `review/aarch64_alu_step4_reconciliation_review.md` judged the
Step 4 deletion on track. The old shard's unary/bit-count/bswap/popcount,
unsigned div/rem reduction, accumulator fallback, register-direct scratch, and
i128-copy text-emitter guidance is legacy-only and not live compiled ALU
behavior. Before lifecycle closure, plan-owner should decide whether that
legacy guidance needs a durable follow-up idea, because `todo.md` is not
durable after close.

## Suggested Next

Hand lifecycle state to plan-owner to decide whether to close the active idea
or create a durable follow-up idea for the deferred legacy-only ALU guidance
before closing.

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

Ran final supervisor validation:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure`.

Result: passed. `test_after.log` contains the combined successful build and
full-suite output: 3167/3167 tests passed.
