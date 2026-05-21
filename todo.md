# Current Packet

Status: Active
Source Idea Path: ideas/open/368_aarch64_00217_c_c_behavior_runtime_mismatch.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Localize Post-Semantic Boundary

## Just Finished

Step 1 localized the first incorrect post-semantic boundary for
`c_testsuite_aarch64_backend_src_00217_c` to AArch64 pointer-value
`StoreLocalInst` writeback/lowering, specifically the indirect store for
`bir.store_local %t9.store.addr, i32 %t13, addr %t9`.

Evidence:

- Semantic BIR admission still succeeds and contains the intended value flow:
  `%t6 = bir.sub i64 %t5, %t4`, `%t9 = bir.add ptr %t7, %t8`,
  `%t10 = bir.load_local i32 %t10.addr, addr %t9`,
  `%t12 = bir.add i64 %t11, %t6`, `%t13 = bir.trunc i64 %t12 to i32`,
  then `bir.store_local %t9.store.addr, i32 %t13, addr %t9`.
- Prepared BIR preserves that same operation. Its prepared-addressing record
  for instruction 15 is `base=pointer_value stored=%t13 pointer=%t9 offset=0
  size=4 align=4`, so the prepared boundary correctly names `%t13` as the
  stored value and `%t9` as the pointer-derived address.
- Generated AArch64 computes the correct update into `w13` with
  `add x9, x9, x13` followed by `mov w13, w9`, but the indirect store sequence
  does not store `w13`. It reloads the target and locals, effectively computes
  `b + (a - b) == a`, then stores `w9`, producing byte `0x05` at `data + 4`.
- Runtime proof confirms the old semantic handoff failure did not return:
  `backend_lir_to_bir_notes`, `00005`, and `00173` pass, while only `00217`
  fails with expected `data = "0123-5678"` and actual `data = "0123\x05"`.

Candidate owner: AArch64 backend pointer-value local-store writeback/publication
for stack-homed pointer addresses and named scalar stored values, likely around
the `StoreLocalInst` address-materialization/writeback path rather than
semantic BIR, prepared BIR, ABI, global data publication, or runtime execution.

## Suggested Next

Supervisor should select a Step 2 packet to add focused coverage for an AArch64
pointer-value `StoreLocalInst` that stores a previously computed named scalar
through a stack-homed pointer-derived address, without naming `00217` or
matching the final output text.

## Watchouts

- Do not reopen semantic `lir_to_bir` indirect local-memory admission unless
  the old semantic handoff failure returns.
- Do not repair this by recomputing the source expression during store
  emission; the prepared contract already provides `%t13` as the value to
  store.
- The suspicious emitted sequence is after `mov w13, w9` in
  `build/c_testsuite_aarch64_backend/src/00217.c.s`: it reloads `[sp, #72]`,
  `[sp, #24]`, and `[sp, #16]`, then stores `w9` through `[sp, #72]` instead
  of storing the already materialized `w13`.
- Do not special-case `00217`, `data`, exact output text, one cast spelling,
  one offset, one generated temporary, or one emitted instruction neighborhood.
- Do not edit expectations, unsupported classifications, runner behavior,
  timeout policy, CTest registration, proof-log policy, or external contracts.
- Keep idea 367 stability checks visible: `backend_lir_to_bir_notes`, `00005`,
  and `00173`.
- Existing untracked files under `review/` are transient artifacts and were not
  touched during activation.

## Proof

`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_lir_to_bir_notes|c_testsuite_aarch64_backend_src_00005_c|c_testsuite_aarch64_backend_src_00173_c|c_testsuite_aarch64_backend_src_00217_c)$' > test_after.log 2>&1`

Result: build was up to date; CTest returned nonzero because
`c_testsuite_aarch64_backend_src_00217_c` still fails with the known runtime
mismatch. The delegated stability subset passed:
`backend_lir_to_bir_notes`, `c_testsuite_aarch64_backend_src_00005_c`, and
`c_testsuite_aarch64_backend_src_00173_c`. Proof log: `test_after.log`.
