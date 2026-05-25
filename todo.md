Status: Active
Source Idea Path: ideas/open/03_dispatch_responsibility_reduction.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Extract Value Materialization Ownership

# Current Packet

## Just Finished

Step 2 - Extract Value Materialization Ownership inspection found one remaining
small Step 2 extraction target after the GOT load move:
`lower_scalar_mul_with_distinct_rhs_scratch`.

Reasoning tied to the current files:

- `dispatch.cpp` still owns `lower_scalar_mul_with_distinct_rhs_scratch`, and
  AST caller inspection shows its only direct caller is `dispatch_prepared_block`.
- The helper is scalar value materialization, not call-source logic: it selects
  prepared result storage, publishes `Mul` operands into GPRs, emits `lsl` for
  power-of-two immediates or `mul` otherwise, writes stack homes when needed,
  and records the emitted scalar register.
- `dispatch_value_materialization.cpp` already contains the closely related
  operand publication machinery for scalar binary producers, including `Mul`
  publication, `power_of_two_shift`, scratch conflict checks, frame-slot stores,
  and scalar register recording through existing includes.
- The remaining address-materialization, missing compare operand publication,
  same-block producer lookup, and call-boundary paths in `dispatch.cpp` belong
  to later publication/producer/call slices rather than this small Step 2 value
  materialization extraction.

## Suggested Next

Execute the remaining Step 2 extraction by moving
`lower_scalar_mul_with_distinct_rhs_scratch` from `dispatch.cpp` into the
value-materialization owner.

Suggested owned files for the code-changing packet:

- `src/backend/mir/aarch64/codegen/dispatch.cpp`
- `src/backend/mir/aarch64/codegen/dispatch_value_materialization.hpp`
- `src/backend/mir/aarch64/codegen/dispatch_value_materialization.cpp`
- `todo.md`
- `test_after.log`

Suggested focused proof command:

```bash
cmake --build --preset default && ctest --test-dir build -R 'backend_aarch64_instruction_dispatch|backend_aarch64_prepared_scalar_alu_records|backend_aarch64_scalar_alu_records' --output-on-failure > test_after.log 2>&1
```

## Watchouts

- Keep dispatch responsible only for routing the `Mul` fallback branch and
  pushing the returned instruction.
- Preserve both result-home paths: prepared result register and stack-home via
  scratch register plus frame-slot store.
- Preserve operand-order conflict handling via
  `value_publication_may_read_register_index`; this is the capability the
  helper exists to protect.
- Do not touch call-source files or the address-materialization/publication
  helpers in this packet.

## Proof

Inspection-only packet; no build/tests run and no logs touched.
