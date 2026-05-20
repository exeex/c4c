Status: Active
Source Idea Path: ideas/open/348_aarch64_indexed_aggregate_address_writeback.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Add Focused Indexed Aggregate Coverage

# Current Packet

## Just Finished

Step 1 complete: localized the first indexed aggregate address/writeback
boundary for idea 348 from existing AArch64 backend artifacts and logs.

Representative evidence:

- Local multidimensional byte case `00130`: source writes `arr[1][3] = 2`
  and then reads the same element through `arr`, `p`, and `q`. Existing
  `test_after.log` reports `c_testsuite_aarch64_backend_src_00130_c` as
  `[RUNTIME_NONZERO] exit=1`. Generated assembly
  `build/c_testsuite_aarch64_backend/src/00130.c.s` computes nearby addresses
  (`add x13, sp, #1`, `add x21, x13, #3`) but stores the byte with
  `strb w9, [sp, #2]`; the later `p[1][3]` read uses `[x13, #7]`. The first
  bad boundary is prepared address formation/load-store operand selection for
  the selected local element, not only writeback handoff.
- Global indexed swap case `00176`: source `swap(a, b)` should read and write
  `array[a]` and `array[b]`. Existing `test_after.log` reports
  `[RUNTIME_MISMATCH]`: the original unsorted line prints, but the sorted line
  is missing. Generated `swap` sign-extends `a`/`b`, but instead of forming
  `array + index * 4`, it snapshots fixed globals (`array`, `array+4`, ...),
  selects values with compare/csel chains, and writes back to fixed global
  offsets. The first bad boundary is selected address preservation and
  load/store operand selection for dynamic global element access.
- Pointer-parameter case `00181`: source `Move(int *source, int *dest)` must
  scan `source[i]`, scan `dest[j]`, store `dest[j-1] = source[i]`, and clear
  `source[i]`. Existing `test_after.log` reports
  `[RUNTIME_NONZERO] exit=Segmentation fault`. In generated `Move`, the loop
  computes pointer candidates into stack temps, but the condition uses
  `mov x13, x13` instead of loading `*source_i`/`*dest_j`; the element scale is
  also clobbered to `4 * 4` in the scan path, and the expected stores are not
  emitted before `PrintAll`. The first bad boundary is prepared address
  formation plus stale selected-address/value publication for pointer-backed
  indexed aggregate access.
- Struct-member case `00195`: source writes
  `point_array[my_point].x` and `.y`, then prints them. Existing
  `test_after.log` reports `[RUNTIME_MISMATCH]`, expected
  `12.340000, 56.780000`, actual `0.000000, 0.000000`. Generated assembly
  sign-extends `my_point`, then emits fixed `point_array+N` loads/stores and
  stores stale `d9` at those fixed addresses; it does not materialize
  `point_array + my_point * sizeof(struct point) + member_offset` as the
  memory operand. The first bad boundary is load/store operand selection with
  stale temporary publication for indexed struct-member stores.

Overall boundary: the dynamic selected address is not reliably represented as
the authoritative memory operand. The failure appears before final writeback in
address formation and operand selection, then manifests as stale temporary
publication or writes to fixed snapshots. The next route should make focused
tests expose dynamic selected-address preservation across local arrays, globals,
pointer parameters, and struct members before implementation.

## Suggested Next

Execute Step 2 focused coverage: add semantic backend tests that expose the
same selected-address boundary for at least two shapes, preferably local
multidimensional byte or dynamic buffer access plus global indexed swap or
indexed struct-member/pointer-parameter store. Do not start the implementation
repair until the focused tests fail for this boundary or an existing focused
test is proven to cover it.

## Watchouts

- Keep implementation scoped to active focused idea 348, not the parked
  umbrella inventory idea.
- Do not special-case c-testsuite filenames, source symbols, byte offsets,
  temporary registers, or emitted instruction sequences.
- The representative failures include both wrong selected addresses and stale
  value publication. Treat stale `d9`, fixed `array+N`, and wrong local byte
  offsets as symptoms of missing selected-address authority, not as separate
  filename-specific repairs.
- Existing prepared memory operand tests cover record conversion contracts, but
  current evidence does not show focused coverage for dynamic aggregate index
  formation/writeback end to end.
- Keep boolean/comparison, FP expression, semantic admission,
  variadic/byval aggregate call-boundary, scalar cast, scalar local writeback,
  return publication, and direct-call publication work outside this route
  unless fresh first-bad-fact evidence proves the same owner.

## Proof

No build or test rerun was performed; this was a localization packet using
existing artifacts. Evidence read from `test_after.log`,
`build/Testing/Temporary/LastTest.log`, source representatives, and generated
assembly under `build/c_testsuite_aarch64_backend/src/`.

Existing external representative statuses in `test_after.log`:

- `c_testsuite_aarch64_backend_src_00130_c`: runtime nonzero, exit 1.
- `c_testsuite_aarch64_backend_src_00176_c`: runtime mismatch, sorted output
  missing.
- `c_testsuite_aarch64_backend_src_00181_c`: runtime nonzero, segmentation
  fault.
- `c_testsuite_aarch64_backend_src_00182_c`: runtime mismatch, LED buffer
  output mostly blank.
- `c_testsuite_aarch64_backend_src_00187_c`: runtime mismatch for file/buffer
  output.
- `c_testsuite_aarch64_backend_src_00195_c`: runtime mismatch, expected
  `12.340000, 56.780000`, actual `0.000000, 0.000000`.

Proposed narrow proof subset for Step 2/3 after focused coverage is added:

```sh
ctest --test-dir build --output-on-failure -R 'backend_aarch64_(memory_operand_records|prepared_memory_operand_records|memory_operand_contract)|c_testsuite_aarch64_backend_src_001(30|76|81|82|87|95)_c'
```

For a bounded non-timeout reproduction of one representative, use the existing
CTest name directly, for example:

```sh
ctest --test-dir build --output-on-failure -R '^c_testsuite_aarch64_backend_src_00130_c$'
```
