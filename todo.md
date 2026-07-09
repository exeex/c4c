Status: Active
Source Idea Path: ideas/open/648_rv64_call_arg_frame_slot_address_materialization.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Refresh The Call-Argument Source Evidence

# Current Packet

## Just Finished

Lifecycle activation completed for idea 648. No implementation or test files
were changed.

## Suggested Next

Execute Step 1: refresh prepared-BIR, prepared call-plan, RV64 object, and
disassembly diagnostics for `src/20000722-1.c`. Record the exact
`arg.source_selection=local_frame_address_materialization` fact, selected
frame slot, ABI argument register, stale register-home copy, and current owner
before choosing an implementation packet.

## Watchouts

- Do not reopen string-constant local-memory admission.
- Do not special-case `src/20000722-1.c`, `%lv._clit_`, `foo`, `s2`, or `a0`.
- Do not infer frame-slot address materialization from source spelling, stack
  offsets, final assembly, testcase identity, or diagnostics.
- Do not edit expectations, unsupported markers, allowlists, timeouts,
  runtime-comparison policy, or pass/fail accounting.

## Proof

Lifecycle-only activation. No build or ctest proof required. No root-level logs
were created or overwritten.
