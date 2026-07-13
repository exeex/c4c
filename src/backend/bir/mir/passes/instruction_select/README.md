# Instruction Selection

Status: scaffold. Converts semantic MIR operations into target opcode families
and explicit operand constraints while preserving virtual-register identity.

Legacy coverage: all target codegen operation families, constants, casts,
comparisons, selects, memory, atomics, aggregates, intrinsics, inline assembly,
i128/f128, and runtime-helper calls.
