Status: Active
Source Idea Path: ideas/open/203_aarch64_markdown_first_backend_reconstruction.md
Source Plan Path: plan.md
Current Step ID: Step 2
Current Step Title: Extract Old `.cpp` Surfaces To Markdown Artifacts

# Current Packet

## Just Finished

Step 2: Extract Old `.cpp` Surfaces To Markdown Artifacts extracted
`src/backend/mir/aarch64/codegen/globals.cpp` into
`src/backend/mir/aarch64/codegen/globals.md` and removed the old `.cpp`
from the live tree.

The markdown artifact records the old global, label, and thread-local address
materialization contract, including GOT selection, direct `ADRP`/low-12
addressing, `tpidr_el0` TLS handling, dependencies, hidden assumptions, and
rebuild risks.

## Suggested Next

Continue Step 2 with another old AArch64 backend `.cpp` extraction target,
preferably another small codegen surface such as `variadic.cpp` or
`inline_asm.cpp` before moving on to larger surfaces such as `cast_ops.cpp` or
`atomics.cpp`.

## Watchouts

- `globals.cpp` was a fully commented translation surface rather than live
  compiled C++; this packet archived its behavioral contract and deleted the
  obsolete `.cpp`.
- The archived surface has several rebuild hazards: GOT-required globals must
  load through `:got:`/`:got_lo12:`, direct globals and labels need both `ADRP`
  and `:lo12:` `ADD`, TLS globals start from `tpidr_el0` and use
  thread-pointer-relative relocations, and all paths leave the materialized
  address in `x0` for `store_x0_to`.
- Continue keeping Step 2 descriptive. Do not patch or expand remaining old
  AArch64 `.cpp` files while extracting them to markdown.

## Proof

Command:

```bash
bash -lc 'cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R "^backend_"' > test_after.log 2>&1
```

Result: passed. The build completed and the backend CTest subset reported
`100% tests passed, 0 tests failed out of 109`. Log path: `test_after.log`.
