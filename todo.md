Status: Active
Source Idea Path: ideas/open/318_rv64_byval_aggregate_call_abi.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Repair Caller-Side Byval Payload Transport

# Current Packet

## Just Finished

Completed idea 318 Step 3 by repairing the focused fixed-arity RV64 byval
aggregate call route enough for `riscv64_byval_aggregate_fixed_call` to become
positive dump, codegen, and QEMU runtime coverage.

Implemented boundary:

- caller-side byval aggregate-address arguments now gather prepared split local
  aggregate field homes into an ABI-sized temporary payload before the call
- the byval payload gather helper validates range and shape into local pending
  output before committing stack adjustment/copy text, so failed helper checks
  do not leak partial assembly into fallback emission
- later frame-address arguments are materialized against the adjusted stack
  pointer while the temporary payload is live
- callee-side byval pointer formals are stored from their incoming RV64 GPR
  argument register into the prepared stack home
- pointer-value loads can read through stack-homed byval pointer formals
- null pointer local stores are supported for the focused pointer lane

Focused tests now positive:

- `backend_dump_riscv64_byval_aggregate_fixed_call`
- `backend_codegen_route_riscv64_byval_aggregate_fixed_call`
- `backend_rv64_runtime_riscv64_byval_aggregate_fixed_call`

The repair stays limited to fixed-arity integer/pointer byval payload transport
and callee byval home loading. Float/FPR byval lanes, variadic byval transport,
and broader aggregate ABI classification remain outside this packet.

## Suggested Next

Reprobe `src/00140.c` or advance to the next idea 318 step to decide whether
the candidate now passes or exposes a separate float/variadic aggregate ABI
residual.

## Watchouts

- Do not special-case `src/00140.c`, `%p.f`, field names, or fixed stack-slot
  homes.
- Do not fake callee-local aggregate values; caller payload transport must cross
  the call boundary.
- The current byval payload gather uses prepared aggregate member stack-object
  offsets plus BIR call ABI size metadata; it is not a broad aggregate ABI
  rewrite.

## Proof

Focused proof passed 3/3:
`cmake --build --preset default -j && ctest --test-dir build -j --output-on-failure -R 'backend_(dump|codegen_route|rv64_runtime)_riscv64_byval_aggregate_fixed_call'`.

Delegated proof ran:
`cmake --build --preset default -j && ctest --test-dir build -j --output-on-failure -R '^backend_' > test_after.log`.

Result: build passed; backend CTest returned nonzero because the existing
unrelated `backend_riscv_prepared_edge_publication` test still fails. The
focused byval dump/codegen/runtime tests pass in the final proof log. Fresh log:
`test_after.log`.
