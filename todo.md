Status: Active
Source Idea Path: ideas/open/334_aarch64_scalar_machine_node_operand_fact_printing.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Repair The Classified Operand-Fact Owner

# Current Packet

## Just Finished

Step 2 repaired the scalar prepared-operand publication owner. Stack-slot
scalar values can now be represented as prepared frame-slot `MemoryOperand`
facts in both the prepared scalar ALU record path and the fallback selected
machine-node operand path. The fallback path now prefers coherent value
home/storage operands before producer/load-source memory facts, which prevents
register-backed sources such as `%t157` from consuming scratch registers as
local-load memory operands.

Focused coverage added:

- `backend_aarch64_prepared_scalar_alu_records_test` now verifies that a
  stack-slot scalar operand becomes a prepared frame-slot memory operand with
  value id/name, slot id, offset, size, alignment, and selected memory-use
  facts preserved.
- `backend_aarch64_machine_printer_test` now verifies selected scalar ALU
  frame-slot operands materialize before printing for both a `mul` RHS and an
  `add` with two frame-slot sources.

Representative results:

- `c_testsuite_aarch64_backend_src_00214_c` now passes in the delegated proof;
  the old scalar `add` diagnostic
  `scalar add/sub/bitwise memory operands require prepared frame-slot sources`
  is gone.
- `c_testsuite_aarch64_backend_src_00164_c` advances past the old scalar `mul`
  diagnostic `scalar mul/div/rem node has incomplete printable rhs facts` and
  now fails later as a runtime mismatch. First output mismatch is the 8th print
  at `tests/c/external/c-testsuite/src/00164.c:28`, expected `46`, actual
  `-635898024`. The generated assembly for that region reads uninitialized
  stack slots around `sp+#148`, `sp+#152`, and `sp+#156`, so this residual is
  not the repaired scalar operand-fact printer owner.

## Suggested Next

Run plan-owner lifecycle review for Step 2. The source idea's current printer
diagnostics are repaired, one representative passes, and the other has advanced
to a runtime mismatch that appears to belong to a separate value/publication or
local-slot initialization owner.

## Watchouts

Do not reopen parked idea 316 from partial frame evidence unless localization
proves frame allocation is the current first owner. Do not implement a
filename-specific, instruction-index-specific, stack-offset-specific, or
diagnostic-string-only workaround. Keep the unrelated transient
`review/326_stdarg_byval_route_review.md` untouched.

## Proof

Delegated proof command:

`cmake --build build --target c4cll backend_aarch64_scalar_alu_records_test backend_aarch64_prepared_scalar_alu_records_test backend_aarch64_scalar_record_contract_test backend_aarch64_machine_printer_test backend_aarch64_operand_resolution_test backend_aarch64_memory_operand_records_test -j 2 && ctest --test-dir build -j --output-on-failure -R 'backend_aarch64_(scalar_alu_records|prepared_scalar_alu_records|scalar_record_contract|machine_printer|operand_resolution|memory_operand_records)|c_testsuite_aarch64_backend_src_(00164|00214)_c' > test_after.log 2>&1`

Result: build succeeded; CTest selected 8 tests. Passed 7/8:
`backend_aarch64_operand_resolution`,
`backend_aarch64_memory_operand_records`,
`backend_aarch64_scalar_record_contract`,
`backend_aarch64_scalar_alu_records`,
`backend_aarch64_prepared_scalar_alu_records`,
`backend_aarch64_machine_printer`, and
`c_testsuite_aarch64_backend_src_00214_c`. Failed 1/8:
`c_testsuite_aarch64_backend_src_00164_c` with the advanced runtime mismatch
described above. Proof log: `test_after.log`.

`git diff --check` passed.
