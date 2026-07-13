# MIR Call Lowering

Status: scaffold. Realizes typed ABI/call/variadic/runtime-helper plans as
machine call sequences, constrained copies, stack arguments, clobbers, and
return recovery. It must not recompute or override the plans silently.

Legacy coverage: call moves, consumer moves, call-return ABI, formal
publications, variadic calls, tail calls, indirect calls, and helper calls.
