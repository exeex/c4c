Status: Active
Source Idea Path: ideas/open/369_rv64_object_route_terminator_fragment_lowering.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Add Focused Terminator Tests and Implement Semantic RV64 Terminator Emission

# Current Packet

## Just Finished

Completed Steps 2 and 3 of `plan.md` for the next audited terminator shape:
added focused RV64 object-emission tests and implemented semantic object-route
lowering for fused pointer-null conditional branches shaped as
`ne ptr %reg, 0`.

The object emitter now accepts the prepared fused branch-condition facts for
that exact pointer-null shape, materializes null as zero, emits a `bne` to the
true successor plus a local jump to the false successor, and keeps adjacent
pointer compare variants fail-closed.

## Suggested Next

Run Step 5 representative proof for the current object-route terminator state:
rerun the two representative RV64 GCC torture cases (`src/20000224-1.c` and
`src/20000112-1.c`) and audit the next first unsupported shape if any remains.

## Watchouts

- The accepted pointer shape is deliberately narrow: `ne` with a named pointer
  lhs and immediate null rhs. Focused tests keep `eq ptr %reg, 0`,
  `ne ptr %reg, 8`, and `ne ptr %lhs, %rhs` unsupported.
- This packet does not use testcase names or prepared-BIR text matching; it is
  driven by prepared compare/CFG facts and BIR value semantics.

## Proof

Ran the supervisor-selected proof exactly:

`( cmake --build build --target c4cll backend_prepare_frame_stack_call_contract_test backend_prepared_printer_test backend_riscv_object_emission_test && ctest --test-dir build -R '^(backend_prepare_frame_stack_call_contract|backend_prepared_printer|backend_riscv_object_emission)$' --output-on-failure ) > test_after.log 2>&1`

Result: passed. This is sufficient focused proof for the object-emission slice.

Proof log: `test_after.log`
