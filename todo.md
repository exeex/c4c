# AArch64 Backend Non-Leaf Call-Frame LR Preservation Todo

Status: Active
Source Idea Path: ideas/open/285_aarch64_backend_nonleaf_call_frame_lr_preservation.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Confirm Non-Leaf Failure Shape

# Current Packet

## Just Finished

Lifecycle switched from the umbrella inventory to the focused
`aarch64_backend_nonleaf_call_frame_lr_preservation` repair idea. No
implementation work has been done in this lifecycle packet.

## Suggested Next

Execute plan.md Step 1: inspect the AArch64 call/frame/return lowering surfaces,
confirm the generated `00100.c`, `00116.c`, and `00121.c` failure shape, and
identify the narrow proof command for the first code slice.

## Watchouts

- Do not special-case c-testsuite filenames, function names, exact emitted text,
  timeout settings, allowlists, unsupported classifications, or expected
  outputs.
- Keep printf, string literals, variadic calls, loop predicates,
  short-circuiting, aggregates/pointers, static globals, and goto behavior out
  of the first LR preservation repair.
- Use explicit timeouts for runtime probes and check for stale generated
  runtime processes after timeout-oriented runs.

## Proof

Lifecycle-only switch. No build, CTest, runtime execution, implementation edit,
or root proof log was produced.
