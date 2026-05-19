Status: Active
Source Idea Path: ideas/open/302_aarch64_scalar_machine_node_operand_forms.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Inspect Focused Operand Diagnostics

# Current Packet

## Just Finished

Step 1 diagnostic inspection completed for the focused AArch64 scalar
machine-node operand-form subset.

Current focused diagnostics:

- `c_testsuite_aarch64_backend_src_00064_c`: `[FRONTEND_FAIL]`
  `tests/c/external/c-testsuite/src/00064.c`; AArch64 assembly route reached
  the machine-node printer and failed at function 0 block 0 instruction 0:
  `cannot print AArch64 machine node family=scalar opcode=div: scalar
  mul/div/rem node requires register/memory lhs, register/memory/immediate rhs,
  and result register`.
- `c_testsuite_aarch64_backend_src_00139_c`: `[FRONTEND_FAIL]`
  `tests/c/external/c-testsuite/src/00139.c`; AArch64 assembly route reached
  the machine-node printer and failed at function 0 block 0 instruction 3:
  `cannot print AArch64 machine node family=scalar opcode=mul: scalar
  mul/div/rem node requires register/memory lhs, register/memory/immediate rhs,
  and result register`.
- `c_testsuite_aarch64_backend_src_00205_c`: `[FRONTEND_FAIL]`
  `tests/c/external/c-testsuite/src/00205.c`; AArch64 assembly route reached
  the machine-node printer and failed at function 0 block 4 instruction 1:
  `cannot print AArch64 machine node family=scalar
  opcode=logical_shift_right: scalar unsigned reduction node requires register
  lhs, immediate reduction, and result register`.

The shared implementation surface is
`src/backend/mir/aarch64/codegen/alu.cpp`: scalar operand construction and
publication in `make_prepared_scalar_operand`,
`make_prepared_scalar_alu_record`, `make_scalar_fallback_operand`, and
`lower_scalar_instruction`, plus the print-admission predicates in
`make_scalar_alu_print_lines`. `src/backend/mir/aarch64/codegen/instruction.cpp`
`scalar_selection_status` currently marks scalar ALU records selected when the
operation is supported, so malformed operand payloads can survive until the
printer rejects them. `src/backend/mir/aarch64/codegen/machine_printer.cpp`
only dispatches scalar records to the ALU printer and is not the primary repair
surface.

## Suggested Next

Delegate Step 2 for `00064` and `00139` together. They share the
`scalar mul/div/rem` operand-form predicate and should be repaired through the
same scalar arithmetic operand publication/materialization path, not split by
testcase. Leave `00205` for Step 3 unless the Step 2 repair naturally changes
the common scalar fallback/publication helper used by unsigned reductions.

## Watchouts

- The failing printer predicates are grouped checks, so the current diagnostic
  proves the selected node lacks an admitted operand form but does not identify
  which exact payload field is malformed. The public `--dump-mir` and
  `--trace-mir` routes currently report an x86/debug contract-first summary for
  these AArch64 inputs, so a Step 2 implementation may need focused local
  inspection around scalar record construction rather than relying on those
  dumps.
- Keep `00104`, `00182`, `00140`, `00204`, `00216`, runtime buckets, and
  timeout/output-storm buckets out of this owner unless generated-code or
  diagnostic evidence proves shared scalar machine-node operand-form behavior.
- Do not change expectations, allowlists, unsupported classifications, timeout
  policy, runner behavior, CTest registration, or root proof-log policy.
- Do not repair this with filename-specific checks or diagnostic-string
  matching; the next slice should make scalar arithmetic nodes publish printable
  structured operands.

## Proof

Build/proof commands run:

- `cmake --build --preset default`: passed; build was already up to date.
- `ctest --test-dir build -j --output-on-failure -R 'c_testsuite_aarch64_backend_src_(00064|00139|00205)_c'`:
  failed as expected for diagnostic capture; all three focused tests report the
  active machine-node printer diagnostics recorded above.
- Additional diagnostic probes:
  `./build/c4cll --dump-mir --target aarch64-linux-gnu ...` and
  `./build/c4cll --trace-mir --target aarch64-linux-gnu ...` for `00064`,
  `00139`, and `00205`; these returned only the current contract-first
  x86/debug route summary and did not expose AArch64 operand payloads.

No root `test_before.log` or `test_after.log` was created or overwritten for
this diagnostic-only packet.
