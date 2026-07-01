# Frame-Slot Call-Argument Publication

Status: Open
Type: Runtime correctness implementation idea
Parent: `ideas/open/423_rv64_runtime_mismatch_triage.md`
Owning Layer: Prepared call-argument publication before RV64 call lowering
Handoff Directory: `docs/rv64_gcc_torture_post_contract/`
Starting Representative: `src/20080506-2.c`

## Goal

Publish and materialize frame-slot address arguments so RV64 call lowering
passes the intended local-object addresses to callees instead of stale or
uninitialized registers.

## Why This Exists

Runtime triage for active idea 423 found that `src/20080506-2.c` compiles,
links, and runs under c4c, but segfaults under qemu while clang exits `0`.
Semantic BIR passes both local pointer values to `foo`. Prepared BIR identifies
the second argument as a frame-slot address and records
`missing_frame_slot_arg_publication=yes` while also preserving the intended
payload. The linked RV64 binary then passes a wrong or uninitialized register
for the second pointer argument.

This is a producer/prepared publication gap consumed by RV64 call lowering,
not a standalone RV64 memory-instruction bug and not evidence that all
segfault rows share the same owner.

## In Scope

- Start from the first-wrong-edge artifacts for `src/20080506-2.c` in
  `docs/rv64_gcc_torture_post_contract/runtime_first_wrong_edge.md`.
- Repair prepared call-argument publication for frame-slot address arguments
  before RV64 call lowering consumes them.
- Ensure RV64 call lowering uses the published frame-slot address payload
  instead of stale or incidental register state.
- Validate with ordinary-C address-of-local call cases and nearby pointer
  argument materialization probes before accepting the route.
- Treat other segfault rows as probes only after their own first-wrong-edge
  evidence or after this repair can be evaluated without assigning ownership
  by exit mode.

## Out Of Scope

- Changing runtime expectations, output comparisons, allowlists,
  unsupported markers, or pass/fail accounting.
- Assigning every segfault runtime row to this owner based only on qemu signal
  `139`.
- Inferring frame-slot argument facts in RV64 from source names, testcase
  names, local variable names, function names, block indexes, or raw source
  shape.
- Repairing generic RV64 binary operand/value materialization; that belongs to
  the separate follow-up from `src/pr81503.c`.
- Repairing inline asm tied-output/result materialization; that is parked as a
  lower ordinary-C priority follow-up unless explicitly activated later.

## Acceptance Criteria

- `src/20080506-2.c` no longer reaches the recorded missing frame-slot
  argument publication edge, and its c4c RV64 behavior matches clang for the
  focused reproduction.
- Prepared BIR publishes the needed frame-slot address argument authority, and
  RV64 call lowering consumes that authority directly.
- Nearby ordinary-C pointer argument cases show the repair is not limited to
  one testcase or one local variable layout.
- Remaining segfault rows are either left parked with missing proof named or
  reclassified only after independent first-wrong-edge evidence.
- Fresh build proof, focused representative proof, and supervisor-selected
  regression proof are available before closure.

## Reviewer Reject Signals

- Reject any diff whose main effect is to special-case `src/20080506-2.c`,
  functions named `foo` or `main`, local variable `a`, argument index `1`
  without a general frame-slot publication rule, a known block number, or qemu
  signal `139`.
- Reject expectation rewrites, output rewrites, allowlist changes,
  unsupported-marker changes, runtime-comparison changes, or pass/fail
  accounting changes claimed as runtime progress.
- Reject assigning unrelated segfault rows such as `src/pr43008.c` or
  `src/zerolen-1.c` to this owner without independent first-wrong-edge
  evidence.
- Reject RV64-only reconstruction of frame-slot argument payloads from raw
  BIR/source shape when the prepared contract should publish them.
- Reject helper renames, classification-only changes, or log wording changes
  presented as capability repair.
- Reject broad call-lowering rewrites that are not tied to frame-slot
  call-argument publication or that leave the old missing-publication failure
  mode intact behind a new abstraction name.

## Closure Note

Closed after the prepared publication layer began carrying frame-slot address
payload authority for call arguments and RV64 object lowering consumed that
published payload instead of stale register state.

Accepted proof used the object route. Fresh `src/20080506-2.c` validation
showed `payload_frame_slot_present=yes`, `clang_qemu_rc=0`, `c4c_qemu_rc=0`,
and `representative_match=yes`. Focused ordinary-C coverage
`backend_obj_runtime_rv64_frame_slot_pointer_arg_preserves_payload` passed, and
the backend subset passed 331/331.

Text `--codegen asm` remains outside this route's acceptance scope; the
ordinary-C coverage probe recorded that optional text execution still returned
qemu 132 and was intentionally not registered as supported coverage.
