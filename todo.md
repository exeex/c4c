Status: Active
Source Idea Path: ideas/open/203_aarch64_markdown_first_backend_reconstruction.md
Source Plan Path: plan.md
Current Step ID: Step 2
Current Step Title: Extract Old `.cpp` Surfaces To Markdown Artifacts

# Current Packet

## Just Finished

Step 2: Extract Old `.cpp` Surfaces To Markdown Artifacts extracted
`src/backend/mir/aarch64/codegen/comparison.cpp` into
`src/backend/mir/aarch64/codegen/comparison.md` and removed the old `.cpp`
from the live tree.

The markdown artifact records the old comparison role, entry points,
floating-point and integer comparison behavior, fused compare-branch shape,
select behavior, dependencies, hidden assumptions, and rebuild risks.

## Suggested Next

Continue Step 2 with another old AArch64 backend `.cpp` extraction target,
preferably `returns.cpp` or `float_ops.cpp` because each is small enough to
archive cleanly while preserving useful ABI/result-lowering context for the
later rebuild.

## Watchouts

- `comparison.cpp` was a fully commented translation surface rather than live
  compiled C++; this packet archived its behavioral contract and deleted the
  obsolete `.cpp`.
- The archived surface has several rebuild hazards: float comparison condition
  mapping is separate from integer condition-code mapping, the left float
  operand must be preserved before materializing the right operand, `F128`
  comparison delegates to shared soft-float support, fused branch lowering
  uses an inverted branch-around shape, and select invalidates accumulator
  cache after `csel`.
- Continue keeping Step 2 descriptive. Do not patch or expand remaining old
  AArch64 `.cpp` files while extracting them to markdown.

## Proof

Command:

```bash
bash -lc 'cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R "^backend_"' > test_after.log 2>&1
```

Result: passed. The build completed and the backend CTest subset reported
`100% tests passed, 0 tests failed out of 109`. Log path: `test_after.log`.
