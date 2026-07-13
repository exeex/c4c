# Current Packet

Status: Active
Source Idea Path: ideas/open/734_lir_to_new_bir_container_completeness.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Complete globals, strings, externs, symbols and initializers

## Just Finished

- Plan Step 3 now admits producer-valid zero-length dimensions across the
  existing typed fixed-array element families without conflating zero with
  negative unsized/invalid sentinels.
- Exact zero outer and inner dimensions, reconstructed spelling, scalar base,
  pointer depth, source order, and object/initializer facts survive Foundation,
  Raw BIR, and Canonical BIR.

## Suggested Next

- Audit the remaining producer-emitted global `TypeSpec` families and make the
  Step 3 checkpoint decision if no additional coherent family remains.

## Watchouts

- Fixed arrays still require a nonempty bounded dimension vector, exact
  `array_size`/outer-dimension agreement, no computed size expression, and
  exact reconstructed LLVM spelling. Negative `-1`/`-2` dimensions, mirrors,
  aggregate/vector/va-list/function-pointer elements, pointer-to-array and
  reference shapes remain closed.

## Proof

- Passed the supervisor-selected exact proof:
  `cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_' > test_after.log 2>&1`.
- The fresh build completed and root `test_after.log` records all 4/4 backend
  tests passing.
