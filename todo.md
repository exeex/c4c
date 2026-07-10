Status: Active
Source Idea Path: ideas/open/656_rv64_20140828_callee_result_frame_slot_runtime.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Refresh 20140828 Runtime Evidence

# Current Packet

## Just Finished

Lifecycle activation selected
`ideas/open/656_rv64_20140828_callee_result_frame_slot_runtime.md` and created
the active runbook in `plan.md`.

## Suggested Next

Execute Step 1 of `plan.md`: refresh semantic BIR, prepared-BIR, ASM, object,
disassembly, and qemu runtime evidence for
`tests/c/external/gcc_torture/src/20140828-1.c`, then identify the first
downstream wrong-value boundary before implementation.

## Watchouts

- Preserve the `%t6` pointer-source publication and branch RHS authority from
  idea 653.
- Do not special-case `src/20140828-1.c`, `%t6`, `f(a, 1, &d)`, or final
  branch compare shapes.
- Do not change expectations, unsupported markers, allowlists, timeout
  accounting, or runtime policy.

## Proof

Lifecycle-only activation. No code validation was run.
