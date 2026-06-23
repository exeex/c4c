Status: Active
Source Idea Path: ideas/open/313_rv64_external_empty_stub_policy_runtime.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Normalize External Call Policy Evidence

# Current Packet

## Just Finished

Lifecycle activation created this execution state for Step 1 of `plan.md`.

## Suggested Next

Run Step 1 evidence normalization for the primary external-call representatives:
`src/00025.c`, `src/00056.c`, `src/00125.c`, `src/00179.c`, and `src/00220.c`.
Capture emitted assembly, BIR/prepared BIR, symbol behavior, and emit/link/qemu
outcomes before choosing the first implementation boundary.

## Watchouts

- Do not add fake named libc/libm/string/user external bodies.
- Do not classify every external call as unsupported unless supported runtime
  linkage has been considered and preserved where available.
- Reject progress that only removes one observed empty label while bodyless
  executable external stubs remain elsewhere.
- Keep ordinary control, local address, aggregate, and function-pointer work out
  of this route unless it is needed only to isolate an external-call policy
  test.

## Proof

Lifecycle-only activation. No build or CTest run is required for this packet.
