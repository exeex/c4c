Status: Active
Source Idea Path: ideas/open/lir-agent-index-header-hierarchy.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Move HIR-to-LIR Implementation Files Under the Semantic Directory

# Current Packet

## Just Finished

Step 3: Move HIR-to-LIR Implementation Files Under the Semantic Directory
completed the behavior-preserving source relocation:

- Moved `hir_to_lir.cpp`, `const_init_emitter.cpp`, and all
  `stmt_emitter_*.cpp` implementation files under
  `src/codegen/lir/hir_to_lir/`.
- Rewrote moved implementation includes so the private lowering index is local
  to the semantic directory and backend/shared relative includes still resolve.
- Left exported top-level LIR headers and `src/codegen/CMakeLists.txt`
  unchanged; the existing recursive source glob picked up the moved files.

## Suggested Next

Start Step 4: Reassess Call Argument Surfaces by inspecting users of
`call_args.hpp` and `call_args_ops.hpp`, including printer and BIR lowering
users, to confirm whether they remain exported top-level LIR APIs.

## Watchouts

The moved implementation files still include exported LIR/frontend surfaces
through existing include directories. Keep `call_args.hpp` and
`call_args_ops.hpp` exported unless inspection finds implementation-only call
lowering state that belongs under a private `hir_to_lir/call/` subdomain.

## Proof

Ran:

```sh
bash -lc 'cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R "^(frontend_hir_tests|backend_)"' > test_after.log 2>&1
```

Result: passed. Proof log: `test_after.log`.
