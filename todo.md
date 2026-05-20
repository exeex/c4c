Status: Active
Source Idea Path: ideas/open/340_aarch64_scalar_cast_stack_homed_register_source_publication.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Prove Representative And Reclassify Residuals

# Current Packet

## Just Finished

Step 3 refreshed the supervisor-delegated proof and classified the remaining
`00143` failure. The old scalar-cast structured-register-source diagnostic is
absent from `test_after.log`; `00143` now reaches assembly, links, runs, and
fails only as `[RUNTIME_NONZERO]` with exit 1.

Classification evidence points outside scalar-cast source publication. Prepared
BIR for the Duff-device copy still contains the expected fallthrough copy work:
`block_9` through `block_15` each load `a[1]` through `a[7]` and store the
loaded value to `b[1]` through `b[7]` (`/tmp/c4c_00143_prepared_bir.txt`,
lines around 342-412; prepared access records around 4053-4080). Generated
AArch64 assembly for the corresponding fallthrough labels, however, only
updates the `from` and `to` pointer locals and branches onward; it omits the
`ldrh`/`strh` data copy for those cases (`/tmp/c4c_00143.s`, labels
`.LBB1_8`, `.LBB1_10`, `.LBB1_12`, `.LBB1_14`, `.LBB1_16`, `.LBB1_18`, and
`.LBB1_20`). With `count % 8 == 7`, this leaves `b[1]` and later elements
uncopied, so the final verification loop returns 1.

## Suggested Next

Recommend closing or splitting idea 340 after supervisor review: the original
scalar-cast source-publication diagnostic is repaired, and the remaining
representative failure is a generated-code fallthrough/local-store emission
residual. The next coherent implementation packet should target AArch64
emission for BIR blocks that contain fixed-offset local loads/stores after
switch fallthrough, not scalar-cast operand publication.

## Watchouts

- Do not treat the residual as proof that the scalar-cast Step 2 repair is
  incomplete: `%t76 -> %t81` is still present in prepared BIR, asm generation
  succeeds, and the old printer diagnostic is absent.
- Generated verification assembly also compares the loop index register against
  selected `b[n]`; because this testcase initializes `a[n] = n`, the earlier
  actionable mismatch is still the omitted Duff-device copy stores that leave
  `b[]` stale.
- `test_after.log` contains the supervisor-selected proof output. The command
  exits nonzero only because `c_testsuite_aarch64_backend_src_00143_c` fails at
  runtime; `00086`, `00111`, and the four backend tests pass.
- Do not change expectations, unsupported classifications, runner behavior,
  timeout policy, proof-log policy, CTest registration, plan files, or ideas.

## Proof

Ran the exact delegated proof command:

`cmake --build build --target c4cll backend_aarch64_instruction_dispatch_test backend_aarch64_machine_printer_test backend_aarch64_scalar_cast_records_test backend_aarch64_prepared_scalar_cast_records_test -j 2 && ctest --test-dir build -j --output-on-failure -R 'backend_aarch64_(instruction_dispatch|machine_printer|scalar_cast_records|prepared_scalar_cast_records)|c_testsuite_aarch64_backend_src_(00086|00111|00143)_c' > test_after.log 2>&1`

Build completed. `test_after.log` records 6/7 passing tests:
`backend_aarch64_instruction_dispatch`, `backend_aarch64_machine_printer`,
`backend_aarch64_scalar_cast_records`,
`backend_aarch64_prepared_scalar_cast_records`, `00086`, and `00111` pass.
`00143` fails with `[RUNTIME_NONZERO]` exit 1. The old
`scalar cast node requires a structured register source operand` diagnostic is
absent from `test_after.log`.

Classification probes used temporary artifacts only:
`/tmp/c4c_00143_prepared_bir.txt`, `/tmp/c4c_00143_block16_prepared_bir.txt`,
`/tmp/c4c_00143_semantic_bir.txt`, `/tmp/c4c_00143.s`,
`/tmp/c4c_00143_asm_gen.log`, `/tmp/c4c_00143_runtime.out`, and
`/tmp/c4c_00143_runtime.err`.
