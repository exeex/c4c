Status: Active
Source Idea Path: ideas/open/617_scalar_compare_publication.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Repair Scalar Compare Publication Semantics

# Current Packet

## Just Finished

Completed Step 3 (`Repair Scalar Compare Publication Semantics`) as a bounded RV64 object-emission repair.

Changed files:

- `src/backend/mir/riscv/codegen/object_emission.cpp`
- `tests/backend/mir/backend_riscv_object_emission_test.cpp`
- `todo.md`
- `test_after.log`

Implementation summary:

- `fragment_for_prepared_fp_compare_publication(...)` keeps the existing fail-closed prepared GPR result-home requirement, then delegates compare encoding to `append_rv64_fp_compare_to_register(...)`.
- The shared FP compare operand helper now accepts materializable F32/F64 immediates by loading raw bits into a scratch GPR and moving them to a scratch FPR, including non-zero immediates.
- Scratch FPR selection avoids occupied operand homes and uses distinct scratch FPRs when both compare operands require materialization.
- Branch, select, join-carrier, unsupported marker, allowlist, expectation, runtime/accounting, and broad failure-map code were not changed.
- Focused RV64 object-emission tests now cover F32/F64 `Eq`, `Ne`, `Slt`, `Sgt`, `Sle`, and `Sge` scalar compare publication, non-zero F64 immediate compare operands, zero immediate operands, and the existing missing-result-home fail-closed shape.

Seven-row probe under `/tmp/c4c_617_step3_probe` after rebuilding `build/c4cll`:

- `src/20000731-1.c`: pass.
- `src/20011217-1.c`: pass.
- `src/930603-1.c`: pass.
- `src/990117-1.c`: pass.
- `src/gofast.c`: moved to `unsupported_instruction_fragment` at `function=fail`, `instruction_kind=CallInst`, `owner=i32 %t4`.
- `src/loop-8.c`: moved to `prepared_consumer_category=ambiguous_non_parallel_multi_source_stack_destination`.
- `src/strct-pack-1.c`: moved to `[RV64_BACKEND_RUNTIME_MISMATCH]` with `c4c_exit=Segmentation fault`.

## Suggested Next

Proceed to Step 4 close-readiness classification: confirm no current row still has scalar compare publication as first owner, record residual owners, and decide whether idea `617` is close-ready or needs a separate follow-up.

## Watchouts

- Rebuild `build/c4cll` before row probes; the focused object-emission test rebuilds the backend library/test binary but not necessarily the compiler driver used by the C torture runner.
- `src/gofast.c`, `src/loop-8.c`, and `src/strct-pack-1.c` are residuals with separate first owners after this repair, not remaining scalar compare publication rows.
- `test_after.log` is the canonical backend proof log for this packet.

## Proof

Focused proof:

- `cmake --build --preset default --target backend_riscv_object_emission_test && ctest --test-dir build -j --output-on-failure -R '^backend_riscv_object_emission$'` passed.

Delegated proof:

- `cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_' > test_after.log` passed.
- `test_after.log`: `346/346` backend tests passed.
