Status: Active
Source Idea Path: ideas/open/229_aarch64_codegen_markdown_shards_to_cpp.md
Source Plan Path: plan.md
Current Step ID: Step 5
Current Step Title: Reconcile Scalar, Memory, Float, And Wide Operation Shards

# Current Packet

## Just Finished

Completed `plan.md` Step 5 ledger reconciliation for the scalar, memory, float,
and wide-operation shard group: `memory.md`, `cast_ops.md`, `float_ops.md`,
`i128_ops.md`, and `f128.md`. Each entry separates current BIR/prepared/shared
facts from missing AArch64 machine-node work and avoids restoring the archived
accumulator/scratch-register surfaces.

Shard: `src/backend/mir/aarch64/codegen/memory.md`
Classification: real-missing-feature
Current Owner: BIR memory lowering in `src/backend/bir/lir_to_bir/memory/*`;
prepared memory access and address facts in
`src/backend/prealloc/stack_layout/coordinator.cpp`; AArch64 memory record
vocabulary in `src/backend/mir/aarch64/codegen/instruction.hpp` and
`src/backend/mir/aarch64/codegen/instruction.cpp`; partial printer support in
`src/backend/mir/aarch64/codegen/machine_printer.cpp`; no compiled AArch64
dispatch path for memory instructions yet.
Review Result: The shard's old `x0`/`x1`/`x9`/`x10`/`x11`/`x12`/`x17`
lowering surface is stale. Current code already has prepared memory access
facts and structured AArch64 memory records for frame-slot, pointer-value,
symbol, and string bases, but block dispatch only calls scalar lowering for
every retained BIR instruction. Selected memory nodes therefore do not reach
the machine module from prepared load/store instructions, and the terminal
printer only has a narrow store path while explicitly rejecting loads without a
structured destination register. The missing fact belongs in AArch64
load/store machine-node lowering and printer completion; global address
materialization remains idea 233, and memcpy/memset or dynamic-stack helpers
remain separate BIR/AArch64 feature routes.
Proof or Evidence: `dispatch.cpp:93` through `:97` classifies local/global
load/store as `Memory`, but `dispatch.cpp:170` invokes only
`lower_scalar_instruction`; `alu.cpp:307` rejects non-`bir::BinaryInst`
instructions; `instruction.cpp:1010` through `:1084` builds prepared memory
operand records; `instruction.cpp:1670` through `:1708` selects only prepared
frame-slot and pointer-value memory bases; `instruction.cpp:2304` through
`:2465` validates local/global load/store memory identities; `machine_printer.cpp:181`
through `:213` prints stores but rejects load nodes as missing a structured
destination register.
Follow-Up: `ideas/open/234_aarch64_memory_load_store_machine_nodes.md`.

Shard: `src/backend/mir/aarch64/codegen/cast_ops.md`
Classification: real-missing-feature
Current Owner: BIR cast opcode vocabulary in `src/backend/bir/bir.hpp`;
prepared scalar cast record helpers in
`src/backend/mir/aarch64/codegen/instruction.cpp`; no compiled AArch64 dispatch
or printer path for cast machine nodes yet.
Review Result: Simple integer casts have current record vocabulary:
`SExt`, `ZExt`, and `Trunc` can become `ScalarCastRecord`s when prepared
register and storage facts are present. That is only record-level coverage.
Dispatch still sends all instructions through a binary-only scalar lowering
path, so `bir::CastInst` never becomes a selected machine instruction, and the
printer maps sign-extend, zero-extend, and truncate opcodes to no printable
mnemonic. Floating-point casts, pointer/integer casts, bitcasts, and F128 casts
are deferred beyond the simple integer subset. Missing facts belong in AArch64
cast dispatch/selection/printer work, with FP/F128 conversion semantics split
to typed FP and binary128 routes rather than patched into BIR or shared MIR.
Proof or Evidence: `dispatch.cpp:86` through `:88` classifies `bir::CastInst`
as scalar, but `alu.cpp:307` through `:310` returns no lowering for non-binary
instructions; `instruction.cpp:604` through `:622` accepts only simple integer
cast opcodes; `instruction.cpp:660` through `:680` maps unsupported casts to
`Deferred`; `instruction.cpp:2233` through `:2301` builds prepared scalar cast
records; `instruction.cpp:1647` through `:1665` can select supported simple
integer casts; `machine_printer.cpp:222` through `:225` only prints scalar
add/sub and rejects other scalar opcodes.
Follow-Up: `ideas/open/235_aarch64_scalar_cast_and_float_machine_nodes.md`.

Shard: `src/backend/mir/aarch64/codegen/float_ops.md`
Classification: real-missing-feature
Current Owner: BIR scalar type and binary opcode vocabulary in
`src/backend/bir/bir.hpp`; prepared floating register-bank and ABI placement
facts in `src/backend/prealloc/target_register_profile.cpp` and
`src/backend/prealloc/regalloc.cpp`; no compiled AArch64 FP arithmetic
machine-node owner yet.
Review Result: F32/F64 values are known to BIR and prepared layers, and
prepared register profiles can route float values to FPR/ABI registers. The
compiled AArch64 scalar ALU owner is intentionally integer-only: it accepts
only selected integer binary opcodes and only scalar register views for
`I1`/`I8`/`I16`/`I32`/`I64`/`Ptr`. There are no typed FP/SIMD arithmetic nodes
or printer cases for FADD/FSUB/FMUL/FDIV, and F128 arithmetic belongs to the
binary128 soft-float route. Missing facts belong in AArch64 typed FP
machine-node lowering and register-file transition modeling, not in BIR
opcode invention or the old integer-accumulator bridge.
Proof or Evidence: `target_register_profile.cpp:38` through `:46` recognizes
F32/F64/F128 as float types; `target_register_profile.cpp:75` through `:88`
maps float types to the FPR bank; `regalloc.cpp:50` through `:53` classifies
F32/F64/F128 liveness as `PreparedRegisterClass::Float`; `instruction.cpp:573`
through `:602` limits scalar ALU support to Add/Sub/And/Or/Xor and excludes
Mul/shifts/div/rem/compare; `instruction.cpp:1087` through `:1104` has no
scalar register view for F32/F64/F128; `machine_printer.cpp:215` through
`:294` prints only add/sub scalar nodes.
Follow-Up: `ideas/open/235_aarch64_scalar_cast_and_float_machine_nodes.md`.

Shard: `src/backend/mir/aarch64/codegen/i128_ops.md`
Classification: real-missing-feature
Current Owner: BIR `I128` type vocabulary and ABI legalization in
`src/backend/bir/bir.hpp` and `src/backend/prealloc/legalize.cpp`; partial
target register-bank vocabulary in `src/backend/prealloc/target_register_profile.cpp`;
no prepared ordinary i128 liveness/register ownership and no compiled AArch64
i128 pair-lowering owner yet.
Review Result: The shard describes still-valid wide integer behavior:
low/high pair transport, add/sub carry and borrow, bitwise operations, shifts,
ordered comparisons, and runtime helper calls. Current code can name `I128`
and treats i128 call arguments/results as memory in ABI legalization, but
ordinary register allocation does not classify i128 liveness into a usable
register class, and AArch64 scalar lowering excludes i128 from scalar register
views and selected operations. This is a real wide-integer feature gap spanning
prepared pair/memory carriers and AArch64 pair machine nodes; it should not be
rebuilt as fixed `x0`/`x1` accumulator shortcuts.
Proof or Evidence: `legalize.cpp:57` through `:59` sizes I128/F128 as 16
bytes; `legalize.cpp:125` through `:129` makes I128 call arguments memory;
`legalize.cpp:174` through `:177` makes I128 returns memory; `target_register_profile.cpp:75`
through `:88` maps I128 to the Vreg bank, but `regalloc.cpp:42` through `:57`
classifies ordinary I128 liveness as `PreparedRegisterClass::None`;
`instruction.cpp:1087` through `:1104` rejects I128 as a scalar register view;
`instruction.cpp:573` through `:602` lacks selected mul/shift/div/rem/compare
coverage.
Follow-Up: `ideas/open/236_aarch64_i128_pair_lowering.md`.

Shard: `src/backend/mir/aarch64/codegen/f128.md`
Classification: real-missing-feature
Current Owner: BIR `F128` type vocabulary and ABI/register-bank facts in
`src/backend/bir/bir.hpp`, `src/backend/prealloc/legalize.cpp`,
`src/backend/prealloc/target_register_profile.cpp`, and
`src/backend/prealloc/regalloc.cpp`; no compiled AArch64 binary128 soft-float
transport, source-tracking, helper-call, or machine-node owner yet.
Review Result: F128 is not absent from the shared/prepared model: it has type
size, ABI classification, and FPR/q-register placement vocabulary. The missing
feature is the actual binary128 lowering contract: full 16-byte source
tracking, constants, q-register transport, temporary storage, helper calls,
comparisons, casts, negation, and full-width load/store/copy behavior. Those
facts require prepared/shared carriers plus AArch64 helper-call and memory
transport nodes. They do not belong in scalar F64 lowering, and the archived
`x16`/`x17` scratch convention is not current authority.
Proof or Evidence: `legalize.cpp:57` through `:59` sizes F128 as 16 bytes;
`legalize.cpp:107` through `:117` and `:160` through `:165` classify F128 ABI
paths through float-capable ABI classes on non-x86_64 targets;
`target_register_profile.cpp:38` through `:46` recognizes F128 as a float
type; `target_register_profile.cpp:75` through `:88` maps F128 to the FPR
bank; `target_register_profile.cpp:519` through `:523` returns an AArch64
float result register name for F128 ABI returns; `regalloc.cpp:50` through
`:53` classifies F128 liveness as float; `instruction.cpp:1087` through
`:1104` has no scalar register view for F128, and no AArch64 codegen files
define binary128 helper-call/source-tracking machine nodes.
Follow-Up: `ideas/open/237_aarch64_binary128_softfloat_lowering.md`.

## Suggested Next

Execute `plan.md` Step 6 by reconciling only the target-specific extra shard
group: `atomics.md`, `intrinsics.md`, `inline_asm.md`, and `peephole.md`.

## Watchouts

- Do not mechanically convert markdown shards into same-named C++ files.
- Do not reopen the shared MIR printer boundary closed by idea 224.
- Treat `records.md` as already represented only for the narrow facts proven by
  current compiled owners; its future structured asm/object roadmap text is not
  a Step 3 implementation mandate.
- Do not recreate the stale `asm_emitter.md` inline-asm emitter, scratch
  allocator, template substitution path, or text-first assembler handoff.
- Do not classify a missing feature before checking current compiled owners.
- Memory access facts exist in BIR/prepared layers, but selected AArch64
  load/store dispatch and load printing are still missing; do not re-create the
  legacy memory scratch-register route.
- Cast record helpers are not enough: dispatch and printer support are missing
  for cast nodes.
- F32/F64 arithmetic needs typed FP/SIMD machine nodes, not integer ALU reuse.
- I128 requires prepared pair/memory carriers before AArch64 pair lowering can
  be honest.
- F128 requires full binary128 source tracking and helper-call carriers; do not
  collapse it to scalar F64 behavior.
- Global memory access support is not the same as global, label, GOT, or TLS
  address materialization; keep that with idea 233.
- Keep compatibility projection out of terminal assembly printing; terminal
  assembly must walk shared `module::MachineModule` through
  `mir::print_machine_module` plus the AArch64 `MachineInstructionPrinter`.

## Proof

Ledger-only classification; no build or test proof required, and no broad
validation was run. No `test_after.log` was produced because the delegated
proof explicitly required ledger-only classification.

Evidence inspected with focused `rg`, `sed`, and `nl` reads of:

- `ideas/open/229_aarch64_codegen_markdown_shards_to_cpp.md`
- `src/backend/mir/aarch64/codegen/memory.md`
- `src/backend/mir/aarch64/codegen/cast_ops.md`
- `src/backend/mir/aarch64/codegen/float_ops.md`
- `src/backend/mir/aarch64/codegen/i128_ops.md`
- `src/backend/mir/aarch64/codegen/f128.md`
- `src/backend/bir/bir.hpp`
- `src/backend/mir/aarch64/codegen/dispatch.cpp` / `.hpp`
- `src/backend/mir/aarch64/codegen/alu.cpp` / `.hpp`
- `src/backend/mir/aarch64/codegen/instruction.cpp` / `.hpp`
- `src/backend/mir/aarch64/codegen/machine_printer.cpp` / `.hpp`
- `src/backend/mir/aarch64/codegen/operands.cpp` / `.hpp`
- `src/backend/prealloc/legalize.cpp`
- `src/backend/prealloc/regalloc.cpp`
- `src/backend/prealloc/target_register_profile.cpp` / `.hpp`
- `src/backend/prealloc/stack_layout/coordinator.cpp`
- `src/backend/bir/lir_to_bir/memory/*`
- `ideas/open/233_aarch64_global_address_materialization.md`
- `ideas/open/234_aarch64_memory_load_store_machine_nodes.md`
- `ideas/open/235_aarch64_scalar_cast_and_float_machine_nodes.md`
- `ideas/open/236_aarch64_i128_pair_lowering.md`
- `ideas/open/237_aarch64_binary128_softfloat_lowering.md`
