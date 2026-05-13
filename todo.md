Status: Active
Source Idea Path: ideas/open/203_aarch64_markdown_first_backend_reconstruction.md
Source Plan Path: plan.md
Current Step ID: Step 2
Current Step Title: Extract Old `.cpp` Surfaces To Markdown Artifacts

# Current Packet

## Just Finished

Step 2: Extract Old `.cpp` Surfaces To Markdown Artifacts extracted
`src/backend/mir/aarch64/codegen/asm_emitter.cpp` into
`src/backend/mir/aarch64/codegen/asm_emitter.md` and removed the old `.cpp`
from the live tree.

The markdown artifact records the old inline-asm emitter role, entry points,
constraint behavior, scratch register policy, operand movement rules, memory
operand conventions, dependencies, hidden assumptions, and rebuild risks.

## Suggested Next

Continue Step 2 with another old AArch64 backend `.cpp` extraction target,
preferably a remaining codegen surface that depends on the archived `emit.md`
and `asm_emitter.md` context.

## Watchouts

- `asm_emitter.cpp` was a fully commented translation surface rather than live
  compiled C++; this packet archived its behavioral contract and deleted the
  obsolete `.cpp`.
- The archived surface has several rebuild hazards: incomplete condition-code
  outputs, fixed `x9` bridge use, subtle logical-immediate validation, SP
  special cases, pointer-slot memory operands, and callee-saved scratch
  bookkeeping.
- Continue keeping Step 2 descriptive. Do not patch or expand remaining old
  AArch64 `.cpp` files while extracting them to markdown.

## Proof

Command:

```bash
bash -lc 'cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R "^backend_"' > test_after.log 2>&1
```

Result: passed. The build completed and the backend CTest subset reported
`100% tests passed, 0 tests failed out of 109`. Log path: `test_after.log`.
