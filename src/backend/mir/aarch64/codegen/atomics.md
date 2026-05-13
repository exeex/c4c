# AArch64 Atomic Operations Legacy Surface

This artifact preserves the useful production shape from the removed
`atomics.cpp` translation surface. The old file was not active C++; it was a
line-commented mirror of the reference Rust implementation at
`ref/claudes-c-compiler/src/backend/arm/codegen/atomics.rs`.

## Role

The surface implemented AArch64 lowering for atomic read-modify-write,
compare-exchange, load, store, and fence operations on `ArmCodegen`.

The old lowering used the backend's normal accumulator convention for operand
materialization: pointers, expected values, desired values, and RMW operands
were loaded through `x0`, then copied into fixed temporary registers before the
atomic instruction sequence ran.

Atomic RMW and compare-exchange used AArch64 exclusive load/store loops. Atomic
loads and stores selected acquire/release instruction variants directly from
the requested ordering and scalar width. Fences lowered to `dmb` barriers.

## Entry Points

- `emit_atomic_rmw_impl(dest, op, ptr, val, ty, ordering)`: materializes the
  address and input value, emits an exclusive loop for exchange, test-and-set,
  or arithmetic/bitwise RMW, then stores the old value to `dest`.
- `emit_atomic_cmpxchg_impl(dest, ptr, expected, desired, ty,
  success_ordering, failure_ordering, returns_bool)`: emits an exclusive
  compare-exchange loop and either stores a boolean success flag or the old
  loaded value.
- `emit_atomic_load_impl(dest, ptr, ty, ordering)`: selects an ordinary or
  acquire load instruction, sign-extends signed narrow results, and stores the
  result to `dest`.
- `emit_atomic_store_impl(ptr, val, ty, ordering)`: selects an ordinary or
  release store instruction for the scalar width.
- `emit_fence_impl(ordering)`: lowers non-relaxed orderings to the matching
  AArch64 `dmb` barrier.

## Register Conventions

The old surface used a small fixed register contract:

- `x0`/`w0`: accumulator, old loaded value, and final result carrier.
- `x1`: atomic memory address.
- `x2`/`w2`: RMW input value or compare-exchange expected value.
- `x3`/`w3`: computed RMW temporary or compare-exchange desired value.
- `w4`: status result from exclusive store instructions.

The `exclusive_instrs(ty, ordering)` helper selected three coupled facts:

- exclusive load mnemonic, such as `ldxr`, `ldaxr`, `ldxrb`, or `ldaxrh`
- exclusive store mnemonic, such as `stxr`, `stlxr`, `stxrb`, or `stlxrh`
- register prefix, either `w` for byte/halfword/word atomics or `x` for
  doubleword atomics

## RMW Lowering

`emit_atomic_rmw_impl` first materialized the pointer into `x0`, copied it to
`x1`, then materialized the input value into `x0` and copied it to `x2`.

`Xchg` emitted a loop that repeatedly loaded the old value from `[x1]`, tried
to store the new value from `x2`, and retried while the exclusive store status
in `w4` was nonzero.

`TestAndSet` followed the same exclusive-loop shape, but stored immediate `1`
through `w3` rather than using the caller-provided input value.

All other RMW operations loaded the old value, delegated operation-specific
arithmetic or bitwise emission to `emit_atomic_op_arm(state, op, tmp, old,
val)`, then retried the exclusive store of the computed temporary until it
succeeded.

The RMW result contract was the old loaded value in `x0`/`w0`, which was stored
to the destination after the loop.

## Compare-Exchange Lowering

`emit_atomic_cmpxchg_impl` materialized the pointer into `x1`, desired value
into `x3`, and expected value into `x2`. It then built three labels:

- `.Lcas_loop_<id>` for the exclusive-load retry loop
- `.Lcas_fail_<id>` for the compare failure path
- `.Lcas_done_<id>` for the common exit

The loop loaded the old value, compared it with the expected register, branched
to failure on mismatch, and tried the exclusive store of the desired register
on match. A failed store status in `w4` retried the loop.

When `returns_bool` was true, success wrote `x0 = 1` and failure wrote
`x0 = 0`. When `returns_bool` was false, the old loaded value already remained
in `x0`/`w0`. The failure path emitted `clrex` before exiting.

The failure ordering argument was intentionally unused in this old surface; all
instruction selection came from `success_ordering`.

## Atomic Load And Store Selection

Loads used acquire forms for `Acquire`, `AcqRel`, and `SeqCst`, and ordinary
loads for `Relaxed` and `Release`:

- 8-bit: `ldarb` or `ldrb`
- 16-bit: `ldarh` or `ldrh`
- 32-bit: `ldar` or `ldr` into `w0`
- wider/default: `ldar` or `ldr` into `x0`

Signed loads normalized the accumulator after loading:

- `I8`: `sxtb x0, w0`
- `I16`: `sxth x0, w0`
- `I32`: `sxtw x0, w0`

Stores used release forms for `Release`, `AcqRel`, and `SeqCst`, and ordinary
stores for `Relaxed` and `Acquire`:

- 8-bit: `stlrb` or `strb`
- 16-bit: `stlrh` or `strh`
- 32-bit: `stlr` or `str` from `w1`
- wider/default: `stlr` or `str` from `x1`

## Fence Lowering

The old fence mapping was:

- `Relaxed`: no instruction
- `Acquire`: `dmb ishld`
- `Release`: `dmb ishst`
- `AcqRel` and `SeqCst`: `dmb ish`

## Dependencies

The removed surface depended on these surrounding concepts:

- IR model: `AtomicOrdering`, `AtomicRmwOp`, `Operand`, `Value`, and `IrType`.
- Backend operand materialization and destination storage helpers:
  `operand_to_x0` and `store_x0_to`.
- Backend assembly emission through `state.emit` and `state.emit_fmt`.
- Stable label generation through `state.next_label_id`.
- Atomic instruction selection through `exclusive_instrs`.
- Operation-specific RMW lowering through `emit_atomic_op_arm`.
- AArch64 exclusive monitor instructions and `clrex`.

## Hidden Assumptions

- Operand materialization through `operand_to_x0` is safe before copying each
  value into its fixed temporary register.
- The pointer remains live in `x1` for the full exclusive loop.
- `exclusive_instrs` must choose byte, halfword, word, or doubleword mnemonics
  consistently with the register prefix used for all loop operands.
- The old RMW contract returns the pre-update value, not the newly computed
  value.
- Compare-exchange success can overwrite `x0` with a boolean only when
  `returns_bool` is true; otherwise the old loaded value must remain available
  for storage.
- Compare-exchange failure must clear the exclusive monitor before leaving the
  loop.
- The failure ordering parameter was not represented in the emitted sequence.
- Signed atomic loads require explicit sign-extension before destination
  storage; unsigned narrow loads rely on the zeroing behavior of `w0` writes.
- `AcqRel` on a plain load was treated like acquire, and `AcqRel` on a plain
  store was treated like release.

## Known Failure Risks For Rebuild

- Returning the computed RMW value instead of the old loaded value would break
  atomic fetch operation semantics.
- Mixing register widths between `exclusive_instrs`, loaded values, store
  values, and compare operands would produce invalid or semantically wrong
  AArch64 sequences.
- Forgetting to retry on nonzero exclusive-store status would make RMW and CAS
  non-atomic under contention.
- Omitting `clrex` on the compare-exchange failure path could leave the
  exclusive monitor live after an early exit.
- Rebuilding compare-exchange without deciding how to represent failure
  ordering would preserve an old semantic gap.
- Dropping signed load extension would corrupt later consumers that read the
  result as a signed 64-bit value.
- Treating every `SeqCst` operation as only acquire or only release may be
  insufficient if the rebuilt backend adopts a stronger memory-ordering model
  than this legacy surface.
- The fixed temporary-register convention can conflict with any later calling
  convention or register allocator integration unless the rebuilt lowering owns
  those clobbers explicitly.

## Rebuild Guidance

Rebuild this surface around explicit atomic operation contracts:

Atomic lowering should model ordering, width, success/failure result mode, and
exclusive-loop structure in target MIR and machine instruction nodes. Exclusive
load/store mnemonics are derived printer or encoder data, not the source of
atomic semantics.

1. Keep the old/result-value contract for RMW and compare-exchange separate
   from the store/retry mechanics.
2. Centralize exclusive instruction selection so width, ordering, and register
   prefix cannot drift apart.
3. Make compare-exchange result mode explicit: boolean result versus old-value
   result.
4. Decide and document how failure ordering should affect compare-exchange
   codegen before treating this as complete semantic coverage.
5. Keep signed load normalization and fence ordering mappings visible in tests
   or contract documentation.
