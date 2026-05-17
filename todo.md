Status: Active
Source Idea Path: ideas/open/264_backend_and_aarch64_codegen_entrypoint_clarity.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Align Route Documentation And Contracts

# Current Packet

## Just Finished

Step 4 aligned the AArch64 route documentation and contract text with the
current ownership model: `backend.cpp` driver prepares BIR, public
`codegen.hpp` exposes `compile_prepared_module(...)`, internal
`module_compile.{hpp,cpp}` coordinates traversal/dispatch/family lowerers, and
`asm_emitter` plus the shared MIR printer owns current `.s` rendering.

Stale current-route references to live `codegen/emit.*` were updated or marked
as legacy artifact names, and the contracts now state that `.s` text is one
downstream consumer format rather than the only reusable AArch64 codegen
product.

## Suggested Next

Step 5 should validate the active route and prepare close review. Suggested
focus: compare `backend.cpp`, `codegen.hpp`, `module_compile.{hpp,cpp}`, and
the updated docs/contracts against the source idea completion criteria, then
run the supervisor-selected broader proof.

## Watchouts

- This was a docs/contract-only packet. No implementation sources, CMake,
  tests, `plan.md`, or source ideas were changed.
- `codegen/README.md` still preserves historical module-surface notes by
  design; its new front matter is the live-route authority for this packet.

## Proof

Supervisor-selected Step 4 proof passed and is preserved in `test_after.log`:

`{ cmake --build build --target backend_aarch64_signature_metadata_test backend_aarch64_prepared_handoff_gate_test && ctest --test-dir build -R '^(backend_aarch64_signature_metadata|backend_aarch64_prepared_handoff_gate)$' --output-on-failure; } > test_after.log 2>&1`
