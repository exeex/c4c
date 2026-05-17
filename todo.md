Status: Active
Source Idea Path: ideas/open/269_aarch64_peephole_markdown_shard_implementation_redistribution.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Add The Compiled Peephole Owner

# Current Packet

## Just Finished

Completed plan Step 2, "Add The Compiled Peephole Owner", by adding the
compiled AArch64 peephole owner as a deferred/no-op boundary.

Added `src/backend/mir/aarch64/codegen/peephole.hpp` with the narrow
`apply_deferred_peephole_boundary(std::string assembly)` API and added
`src/backend/mir/aarch64/codegen/peephole.cpp` with an unchanged move-return
implementation. The implementation documents that this owner records the
current no-live-pass decision while archived text-first rewrites remain
deferred until a structured or explicit printer-output cleanup policy lands.

Added `peephole.cpp` to `src/backend/CMakeLists.txt` so the new owner compiles.
No output path was wired to the boundary in this step, so emitted assembly
behavior remains unchanged.

## Suggested Next

Execute the next supervisor-selected packet for the AArch64 peephole
redistribution plan. A likely next packet is a review or integration-planning
slice that decides when, if ever, the deferred boundary should be called by an
explicit output policy.

## Watchouts

Do not reintroduce the legacy text optimizer, RISC-V-style `peephole_optimize`
surface, string classifier, global copy propagation, or dead stack-store
passes. Step 2 intentionally did not make `asm_emitter`, `machine_printer`, or
`backend.cpp` hidden peephole owners.

## Proof

Ran `cmake --build --preset default > test_after.log 2>&1`; it passed.
`test_after.log` is the proof log path.
