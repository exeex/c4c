Status: Active
Source Idea Path: ideas/open/lir-agent-index-header-hierarchy.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Create the Private HIR-to-LIR Lowering Index

# Current Packet

## Just Finished

Step 2: Create the Private HIR-to-LIR Lowering Index completed the scoped
private header relocation without behavior changes:

- Added `src/codegen/lir/hir_to_lir/lowering.hpp` as the private HIR-to-LIR
  lowering index.
- Moved the implementation-only `StmtEmitter`, `stmt_emitter_detail`, and
  `ConstInitEmitter` declarations into that private index.
- Rewrote HIR-to-LIR implementation files to include
  `hir_to_lir/lowering.hpp` instead of the retired top-level private headers.
- Removed obsolete top-level `src/codegen/lir/stmt_emitter.hpp` and
  `src/codegen/lir/const_init_emitter.hpp`.

## Suggested Next

Start Step 3 by moving HIR-to-LIR implementation files under
`src/codegen/lir/hir_to_lir/`, updating build definitions and include paths as
a behavior-preserving path-depth cleanup.

## Watchouts

The new private index still includes exported `call_args.hpp` because call
argument utilities remain cross-component APIs. Step 3 should keep source moves
mechanical and avoid creating one header per moved implementation file.

## Proof

Ran:

```sh
bash -lc 'cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R "^frontend_hir_tests$"' > test_after.log 2>&1
```

Result: passed. Proof log: `test_after.log`.
