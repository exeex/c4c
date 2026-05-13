Status: Active
Source Idea Path: ideas/open/203_aarch64_markdown_first_backend_reconstruction.md
Source Plan Path: plan.md
Current Step ID: Step 2
Current Step Title: Extract Old `.cpp` Surfaces To Markdown Artifacts

# Current Packet

## Just Finished

Step 2: Extract Old `.cpp` Surfaces To Markdown Artifacts extracted
`src/backend/mir/aarch64/codegen/prologue.cpp` into
`src/backend/mir/aarch64/codegen/prologue.md` and removed the old `.cpp`
from the live tree.

The markdown artifact records the old prologue/epilogue role, entry points,
stack layout behavior, variadic save-area accounting, callee-saved register
save policy, parameter storage and `ParamRef` behavior, dependencies, hidden
assumptions, and rebuild risks.

## Suggested Next

Continue Step 2 with another old AArch64 backend `.cpp` extraction target,
preferably a remaining codegen surface that depends on the archived `emit.md`,
`asm_emitter.md`, and `prologue.md` context.

## Watchouts

- `prologue.cpp` was a fully commented translation surface rather than live
  compiled C++; this packet archived its behavioral contract and deleted the
  obsolete `.cpp`.
- The archived surface has several rebuild hazards: sret/variadic GP register
  accounting, callee-saved pre-store lifetime extension, F128 caller-saved pool
  suppression, stack-passed parameter addressing via `frame_size + offset`, and
  signed narrow load selection.
- Continue keeping Step 2 descriptive. Do not patch or expand remaining old
  AArch64 `.cpp` files while extracting them to markdown.

## Proof

Command:

```bash
bash -lc 'cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R "^backend_"' > test_after.log 2>&1
```

Result: passed. The build completed and the backend CTest subset reported
`100% tests passed, 0 tests failed out of 109`. Log path: `test_after.log`.
