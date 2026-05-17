Status: Active
Source Idea Path: ideas/open/260_aarch64_float_ops_markdown_shard_implementation_redistribution.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Audit Float Ownership And Boundaries

# Current Packet

## Just Finished

Lifecycle activation created this execution state for Step 1 of `plan.md`.

## Suggested Next

Execute Step 1: audit scalar F32/F64 float ownership across `alu.cpp`, `dispatch.cpp`, `instruction.cpp`, `instruction.hpp`, `machine_printer.cpp`, and current tests before moving code.

## Watchouts

- Keep this behavior-preserving; do not expand FP semantics or weaken tests.
- Keep cast, f128, i128, and unrelated intrinsic shard ownership outside this plan unless the audit proves a direct `float_ops.md` dependency.
- Preserve fail-closed behavior for missing FPR facts, wrong register banks, unsupported opcodes, and F128 operands.

## Proof

No build proof run for lifecycle-only activation.
