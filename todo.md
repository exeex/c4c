Status: Active
Source Idea Path: ideas/open/365_aarch64_signed_remainder_lowering.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Repair Signed Remainder Operand Flow

# Current Packet

## Just Finished

Step 2 added focused machine-printer coverage for signed remainder where the
result aliases the dividend and the divisor is an immediate. The new assertion
requires the emitted sequence to preserve separate quotient and divisor
carriers:
`mov w9, #8`; `sdiv w10, w13, w9`; `msub w13, w10, w9, w13`.

Step 3 repaired `src/backend/mir/aarch64/codegen/alu.cpp` so scalar remainder
printing keeps the materialized RHS scratch as a real occupied
`RegisterOperand`. When a remainder result aliases the dividend, quotient
scratch allocation now excludes the divisor carrier, including immediate and
frame-slot materializations, before emitting `sdiv`/`udiv` and `msub`.

## Suggested Next

Supervisor can review the signed-remainder slice for commit readiness or move
to the next active packet.

## Watchouts

The repair is in the shared signed/unsigned scalar remainder print path and
does not touch switch lowering, synthetic labels, or expectations. Existing
unsigned remainder behavior is preserved while sharing the same divisor-scratch
occupancy protection.

## Proof

Ran the delegated proof:

```sh
cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_.*|c_testsuite_aarch64_backend_src_00143_c)$' > test_after.log 2>&1
```

Result: passed. `test_after.log` reports 144/144 tests passed, including
`backend_.*` and `c_testsuite_aarch64_backend_src_00143_c`.
