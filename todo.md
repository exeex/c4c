Status: Active
Source Idea Path: ideas/open/203_aarch64_markdown_first_backend_reconstruction.md
Source Plan Path: plan.md
Current Step ID: Step 2.5
Current Step Title: Extract top-level `mod.cpp` To Markdown Artifact

# Current Packet

## Just Finished

Step 2.5: Extract top-level `mod.cpp` To Markdown Artifact extracted
`src/backend/mir/aarch64/mod.cpp` into
`src/backend/mir/aarch64/mod.md` and removed the old `.cpp` from the live
tree.

The markdown artifact records the old top-level AArch64 backend module index,
including the `codegen`, `assembler`, and `linker` submodules, the historical
`codegen::emit::ArmCodegen` re-export, the `gcc_assembler` and `gcc_linker`
feature-gate assumptions, cross-surface dependencies, hidden assumptions, and
rebuild risks.

Step 2 is now complete: all pre-existing old `.cpp` files under
`src/backend/mir/aarch64` have been removed from the live tree and preserved as
markdown review artifacts.

## Suggested Next

Next coherent packet: start Step 3 by creating the AArch64 markdown
classification index under `src/backend/mir/aarch64/` or another
supervisor-chosen adjacent docs path.

The index should list every extracted or inspected AArch64 artifact, classify
each as salvageable design note, obsolete route, binary-utils candidate,
target-ABI candidate, assembler/linker candidate, or delete/defer, and mark
which artifacts should not influence the new BIR/prepared backend contract.

## Watchouts

- Step 2 extraction is descriptive only. Do not treat any AArch64 markdown
  artifact as proof that the live C++ backend implements that behavior.
- `mod.md` records the old target-family grouping and the historical
  `ArmCodegen` export spelling. Preserve that as source evidence; do not infer
  a new naming or driver contract from it.
- The extracted assembler and linker artifacts document old built-in toolchain
  paths gated by reference Rust features. The Step 3 index should distinguish
  those routes from current live compiler behavior.
- `mod.hpp`, `parser.hpp`, encoder headers, linker headers, and other headers
  remain in the tree; header deletion or replacement is outside this packet.
- The retargeted signature metadata test remains artifact coverage only; do
  not count it as live AArch64 codegen, assembler, or linker behavior proof.

## Proof

Command:

```bash
bash -lc 'cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R "^backend_"' > test_after.log 2>&1
```

Result: passed. The build completed and the backend CTest subset reported
`100% tests passed, 0 tests failed out of 109`. Log path: `test_after.log`.
