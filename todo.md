Status: Active
Source Idea Path: ideas/open/324_rv64_duff_fallthrough_pointer_update_producers.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Repair Semantic/Prepared Pointer Producers

# Current Packet

## Just Finished

Completed Step 3 semantic/prepared pointer producer repair for Duff-style
fallthrough pointer updates.

Updated RV64-targeted LIR-to-BIR GEP lowering so pointer-value GEPs that publish
their result back into a local pointer slot can materialize an explicit
`bir.add ptr` when the containing suffix participates in a loop/fallthrough
cycle. This preserves the already-green prepared local-array pointer-step
neighbor by requiring the suffix to close to the containing block or an earlier
block, rather than authorizing every same-block pointer-local reassignment.

Converted `backend_dump_riscv64_duff_fallthrough_pointer_update_producers` from
expected-repair to a positive dump contract. The focused fixture now proves the
later fallthrough pointer-local stores have explicit `bir.add ptr` producers
and `source_producer=binary` records for both source and destination pointer
updates. It still avoids `src/00143.c`, Duff comments, observed stack homes, and
fixed c-testsuite values as behavior contracts.

Quick candidate check: `tests/c/external/c-testsuite/src/00143.c` now shows the
owned block_9 `%t39`/`%t42` pointer updates as explicit `bir.add ptr` operations
with `source_producer=binary` in prepared-BIR. Full RV64 emit/link/qemu reprobe
should remain a Step 4 packet.

## Suggested Next

Reprobe `src/00143.c` through BIR, prepared-BIR, RV64 asm emit, clang link, and
qemu to classify whether the producer repair closes idea 324 or exposes a later
RV64/runtime residual. Add runtime coverage only if the reprobe shows the
current focused fixture can be promoted without folding in a separate route.

## Watchouts

- Do not special-case `src/00143.c`, Duff's-device block names, `%t39`,
  `%t42`, `%lv.from`, `%lv.to`, fixed stack offsets, or fixed array sizes.
- The repair is intentionally semantic/prepared only. Do not claim candidate
  runtime closure until Step 4 reprobes `src/00143.c`.
- The first attempted same-block-publication predicate was too broad and
  changed `riscv64_prepared_local_array_pointer_step`; the accepted rule keeps
  the suffix loop/fallthrough-cycle requirement. Preserve that boundary.
- Entry initialization stores still legitimately report `source=<none>` and
  `source_producer=unknown`; do not use a blanket forbidden snippet for that
  text.

## Proof

Focused proof:
`cmake --build --preset default -j && ctest --test-dir build -j --output-on-failure -R 'backend_(dump|codegen_route|rv64_runtime)_riscv64_(duff_fallthrough_pointer_update_producers|loop_carried_pointer_postincrement|prepared_local_array_pointer_step)'`

Result: passed, 7/7 tests. The corrected dump coverage passed after conversion
to a positive contract, and the idea 323 loop-carried pointer post-increment
plus prepared local-array pointer-step neighbor tests remained positive.

Delegated proof:
`cmake --build --preset default -j && ctest --test-dir build -j --output-on-failure -R '^backend_' > test_after.log`.

Result: build succeeded; backend subset returned nonzero with 285/286 tests
passing (`test_after.log`). The only remaining failure is the existing
`backend_riscv_prepared_edge_publication` contract. The converted Duff
fallthrough dump test and idea 323 loop-carried dump/codegen/runtime tests
passed in the broad run.
