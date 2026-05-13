Status: Active
Source Idea Path: ideas/open/203_aarch64_markdown_first_backend_reconstruction.md
Source Plan Path: plan.md
Current Step ID: Step 2
Current Step Title: Extract Old `.cpp` Surfaces To Markdown Artifacts

# Current Packet

## Just Finished

Step 2: Extract Old `.cpp` Surfaces To Markdown Artifacts extracted
`src/backend/mir/aarch64/codegen/returns.cpp` into
`src/backend/mir/aarch64/codegen/returns.md` and removed the old `.cpp`
from the live tree.

The markdown artifact records the old return-lowering role, entry points,
scalar and binary128 return-register behavior, second floating-point return
component helpers, dependencies, hidden assumptions, and rebuild risks.

## Suggested Next

Continue Step 2 with another old AArch64 backend `.cpp` extraction target,
preferably `float_ops.cpp` because it is small enough to archive cleanly while
preserving useful floating-point operation and `F128` delegation context for
the later rebuild.

## Watchouts

- `returns.cpp` was a fully commented translation surface rather than live
  compiled C++; this packet archived its behavioral contract and deleted the
  obsolete `.cpp`.
- The archived surface has several rebuild hazards: scalar float returns must
  move raw bits from integer temporaries into FP/SIMD ABI registers, `i128`
  returns intentionally leave `x0:x1` untouched, direct `F128` returns and
  `__extenddftf2` widening are separate contracts, and the `F128` second-return
  read path updates backend tracking state after storing `q1`.
- Continue keeping Step 2 descriptive. Do not patch or expand remaining old
  AArch64 `.cpp` files while extracting them to markdown.

## Proof

Command:

```bash
bash -lc 'cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R "^backend_"' > test_after.log 2>&1
```

Result: passed. The build completed and the backend CTest subset reported
`100% tests passed, 0 tests failed out of 109`. Log path: `test_after.log`.
