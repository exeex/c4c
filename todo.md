Status: Active
Source Idea Path: ideas/open/617_scalar_compare_publication.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Prove Family Outcome And Classify Residuals

# Current Packet

## Just Finished

Completed Step 4 (`Prove Family Outcome And Classify Residuals`) as a targeted seven-row family classification after the committed Step 3 scalar compare publication repair.

Fresh targeted probe under `/tmp/c4c_617_step4_probe` after rebuilding `build/c4cll`:

- `src/20000731-1.c`: pass.
- `src/20011217-1.c`: pass.
- `src/930603-1.c`: pass.
- `src/990117-1.c`: pass.
- `src/gofast.c`: reclassified to `unsupported_instruction_fragment`; diagnostic reports `function=fail`, `block=entry`, `instruction_kind=CallInst`, `owner=i32 %t4`.
- `src/loop-8.c`: reclassified to `prepared_consumer_category=ambiguous_non_parallel_multi_source_stack_destination`; diagnostic reports prepared move-bundle classifier rejection for ambiguous non-parallel multi-source stack-destination authority.
- `src/strct-pack-1.c`: reclassified to `[RV64_BACKEND_RUNTIME_MISMATCH]`; `clang_exit=0`, `c4c_exit=Segmentation fault`.

No current seven-row target still has `unsupported_scalar_compare_publication` as first owner. The source idea acceptance criteria are met: scalar compare publication rows progressed or were reclassified with concrete owner evidence, unrelated branch/select/RV64 instruction-fragment routes were not changed by this packet, and proof covers the complete current small family.

## Suggested Next

Recommend plan-owner lifecycle closure for idea `617`.

## Watchouts

- Residual owners are separate follow-up territory: RV64 unsupported call instruction fragment for `src/gofast.c`, move-bundle target/consumer authority for `src/loop-8.c`, and runtime mismatch for `src/strct-pack-1.c`.
- This packet did not modify implementation files, tests, unsupported markers, allowlists, expectations, runtime/accounting files, docs, or broad failure-map artifacts.
- Existing `test_after.log` from Step 3 remains the canonical backend proof log for the code-changing repair; this Step 4 packet did not refresh it.

## Proof

Targeted proof:

- Rebuilt the compiler driver: `cmake --build build --target c4cll`.
- Ran `tests/backend/cmake/run_rv64_gcc_torture_backend_object_case.cmake` directly for all seven Step 1 rows into `/tmp/c4c_617_step4_probe`.
- Probe summary: `4` pass, `3` reclassified to non-scalar-compare owners, `0` remaining `unsupported_scalar_compare_publication` rows.
- Probe logs: `/tmp/c4c_617_step4_probe/summary.tsv` and `/tmp/c4c_617_step4_probe/src_*/case.log`.
- Broader backend proof was not rerun because this packet was diagnostics/classification-only and made no code changes; `test_after.log` was not created or modified by this packet.
