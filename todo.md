# Current Packet

Status: Active
Source Idea Path: ideas/open/535_rv64_object_frame_stack_helper_cleanup.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Map Frame And Stack Helper Surfaces

## Just Finished

Lifecycle activation created the active runbook for Step 1 from
`ideas/open/535_rv64_object_frame_stack_helper_cleanup.md`.

## Suggested Next

Execute Step 1: map the frame sizing, stack-slot offset, register-home lookup,
basic stack load/store, and stack adjustment helper surfaces in
`object_emission.cpp` and `prepared_frame_emit.*`, then record the first safe
implementation boundary in this file.

## Watchouts

- Keep this behavior-preserving: no frame-size, alignment, ABI, diagnostic,
  unsupported-contract, object-byte, expectation, or RV64 capability changes.
- Do not move call-specific byval/sret publication, before-return bundles,
  local memory semantics, broad function traversal, prepared instruction
  dispatch, data-object assembly, relocation handling, or ELF writing.
- Do not infer missing prepared facts from BIR, target text, object bytes, or
  testcase shape.

## Proof

Lifecycle-only activation; no build run.
