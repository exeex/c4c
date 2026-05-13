Status: Active
Source Idea Path: ideas/open/203_aarch64_markdown_first_backend_reconstruction.md
Source Plan Path: plan.md
Current Step ID: Step 2
Current Step Title: Extract Old `.cpp` Surfaces To Markdown Artifacts

# Current Packet

## Just Finished

Step 2: Extract Old `.cpp` Surfaces To Markdown Artifacts extracted
`src/backend/mir/aarch64/codegen/emit.cpp` into
`src/backend/mir/aarch64/codegen/emit.md` and removed the old `.cpp` from the
live tree.

The markdown artifact records the old public entry points, target-profile
normalization behavior, BIR and LIR dispatch order, minimal slice recognizers,
handwritten assembly emitters, broad `gen_*` general-LIR renderer surface,
dependencies, hidden assumptions, and rebuild risks.

## Suggested Next

Continue Step 2 with the next AArch64 codegen extraction target:
`src/backend/mir/aarch64/codegen/asm_emitter.cpp`. It should clarify raw
assembly text conventions after the central `emit.cpp` surface has been
archived.

## Watchouts

- The old `emit.cpp` was not present in `build/compile_commands.json`, so the
  compile-database clang query could not load it; extraction used targeted
  structural searches and focused reads instead.
- `emit.hpp` still declares the legacy AArch64 emitter entry points; this
  packet did not own header/API cleanup.
- The extracted surface shows strong overfit pressure: many narrow BIR/LIR
  shape recognizers and stringly type parsers should not be regrown during the
  rebuild.
- Continue keeping Step 2 descriptive. Do not patch or expand remaining old
  AArch64 `.cpp` files while extracting them to markdown.

## Proof

Command:

```bash
bash -lc 'cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R "^backend_"' > test_after.log 2>&1
```

Result: passed. The build completed and the backend CTest subset reported
`100% tests passed, 0 tests failed out of 109`. Log path: `test_after.log`.
