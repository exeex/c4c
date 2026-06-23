Status: Active
Source Idea Path: ideas/open/324_rv64_duff_fallthrough_pointer_update_producers.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Add Focused Duff Fallthrough Producer Coverage

# Current Packet

## Just Finished

Completed Step 2 focused expected-repair coverage for missing Duff/fallthrough
pointer update producer publication.

Added `tests/backend/case/riscv64_duff_fallthrough_pointer_update_producers.c`,
a small switch/fallthrough copy fixture with local pointer post-increments. It
is independent of `src/00143.c`, the Duff comment/source spelling, candidate
block names, observed value IDs, observed stack offsets, and fixed candidate
array sizes.

Added `backend_dump_riscv64_duff_fallthrough_pointer_update_producers` to
`tests/backend/CMakeLists.txt`. The corrected expected-repair dump test proves
both sides of the boundary:

- the already-repaired idea 323 shape still exists in the same nearby feature
  family through `bir.add ptr` and `source_producer=binary` pointer-local
  publication;
- a later fallthrough copy/update block currently stores next pointer values
  into pointer locals with `source_producer=unknown`, while still having
  pointer-value load/store memory accesses.

This is intentionally dump-only pre-repair coverage. Runtime/codegen positive
coverage should wait until the missing producer facts are repaired; the current
test preserves the gap without encoding c-testsuite filenames or fixed stack
homes. The existing dump harness supports literal snippets only, so the
expected-repair assertion pins the focused fixture's concrete fallthrough
store-source records (`block_7` pointer-local stores) to avoid the earlier false
positive from unrelated `source=<none>` initialization stores.

## Suggested Next

Repair semantic/prepared publication for Duff-style fallthrough pointer updates
so later fallthrough copy blocks publish next-pointer producer facts analogous
to the first repaired loop-carried block. Then flip the expected-repair dump
coverage to a positive contract and add/enable runtime proof if supported.

## Watchouts

- Do not special-case `src/00143.c`, Duff's-device block names, `%t39`,
  `%t42`, `%lv.from`, `%lv.to`, fixed stack offsets, or fixed array sizes.
- Do not teach RV64 to guess from uninitialized stack homes or synthesize
  pointer updates without prepared semantic authority.
- Do not reopen idea 323's RV64 consumption path while later fallthrough blocks
  still have unknown source producers.
- Keep runtime proof on the supported path; do not weaken qemu coverage or
  mark the candidate unsupported.
- Keep the focused fixture semantic. The test currently checks the specific
  prepared missing-producer boundary for the later fallthrough pointer stores;
  do not convert it to a runtime-only crash, a generic `source_producer=unknown`
  match, or an offset-shaped assertion.

## Proof

Focused proof:
`cmake --build --preset default -j && ctest --test-dir build -j --output-on-failure -R 'backend_(dump|codegen_route|rv64_runtime)_riscv64_(duff_fallthrough_pointer_update_producers|loop_carried_pointer_postincrement|prepared_local_array_pointer_step)'`

Result: passed, 7/7 tests. The corrected expected-repair dump coverage passed
with snippets tied to the later fallthrough pointer-local store-source records,
and the idea 323 loop-carried pointer post-increment plus prepared local-array
pointer-step neighbor tests remained positive.

Delegated proof:
`cmake --build --preset default -j && ctest --test-dir build -j --output-on-failure -R '^backend_' > test_after.log`.

Result: build succeeded; backend subset returned nonzero with 284/285 tests
passing before this packet; after adding the expected-repair dump test the
backend subset returned nonzero with 285/286 tests passing (`test_after.log`).
The only remaining failure is the existing `backend_riscv_prepared_edge_publication`
contract. The new expected-repair dump test and idea 323 loop-carried
dump/codegen/runtime tests passed in the broad run.
