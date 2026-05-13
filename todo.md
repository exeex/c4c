Status: Active
Source Idea Path: ideas/open/203_aarch64_markdown_first_backend_reconstruction.md
Source Plan Path: plan.md
Current Step ID: Step 2
Current Step Title: Extract Old `.cpp` Surfaces To Markdown Artifacts

# Current Packet

## Just Finished

Step 2: Extract Old `.cpp` Surfaces To Markdown Artifacts extracted
`src/backend/mir/aarch64/codegen/float_ops.cpp` into
`src/backend/mir/aarch64/codegen/float_ops.md` and removed the old `.cpp`
from the live tree.

The markdown artifact records the old floating-point operation role, entry
points, scalar `F32`/`F64` register-file transitions, `F128` soft-float
delegation, dependencies, hidden assumptions, and rebuild risks.

## Suggested Next

Continue Step 2 with another old AArch64 backend `.cpp` extraction target,
preferably `globals.cpp` because it is small and can preserve global symbol
addressing behavior before moving on to larger surfaces such as `cast_ops.cpp`
or `atomics.cpp`.

## Watchouts

- `float_ops.cpp` was a fully commented translation surface rather than live
  compiled C++; this packet archived its behavioral contract and deleted the
  obsolete `.cpp`.
- The archived surface has several rebuild hazards: scalar `F32`/`F64` binary
  ops bridge raw bits through `x0`, preserve the left operand in `x1` before
  evaluating the right operand, move through `s`/`d` FP registers for native
  arithmetic, explicitly zero-extend `F32` results with `mov w0, w0`, and keep
  `F128` binary ops plus negation delegated to shared binary128 helpers.
- Continue keeping Step 2 descriptive. Do not patch or expand remaining old
  AArch64 `.cpp` files while extracting them to markdown.

## Proof

Command:

```bash
bash -lc 'cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R "^backend_"' > test_after.log 2>&1
```

Result: passed. The build completed and the backend CTest subset reported
`100% tests passed, 0 tests failed out of 109`. Log path: `test_after.log`.
