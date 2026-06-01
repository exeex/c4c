# Current Packet

Status: Active
Source Idea Path: ideas/open/80_aarch64_dispatch_publication_owner_relocation.md
Source Plan Path: plan.md
Current Step ID: Step 2
Current Step Title: Relocate one narrow owner-local publication family

## Just Finished

Completed Step 2 from `plan.md`: relocated the memory-owned result recording
and prepared-home retarget helpers out of dispatch publication and into the
AArch64 memory owner. `record_memory_result` and
`retarget_memory_result_to_prepared_home` are now declared from `memory.hpp`
and defined in `memory.cpp`; dispatch call sites keep the same semantics via
the memory owner surface.

Changed files:

- `src/backend/mir/aarch64/codegen/dispatch_publication.cpp`
- `src/backend/mir/aarch64/codegen/dispatch_publication.hpp`
- `src/backend/mir/aarch64/codegen/memory.cpp`
- `src/backend/mir/aarch64/codegen/memory.hpp`
- `todo.md`

The retarget path still consumes existing prepared value-location,
value-home, ABI-move, and storage-plan facts. It does not re-derive prepared
storage or value facts locally, and no store-publication or current-block join
helpers were pulled into memory.

## Suggested Next

Execute the next Step 2/Step 3 relocation packet for a narrow remaining
owner-local publication helper family, with supervisor selection based on the
remaining `dispatch_publication.*` surface.

## Watchouts

- `record_address_materialization_result` still lives in
  `dispatch_publication.cpp`; do not fold it into memory unless a later packet
  explicitly classifies address materialization ownership.
- Store retarget/publication helpers still remain on the dispatch publication
  surface and may have memory-adjacent names, but this packet intentionally did
  not move them.
- Root-level `test_before.log` and `test_baseline.log` were already present
  alongside the required `test_after.log`; this packet wrote only
  `test_after.log`.

## Proof

Ran the supervisor-selected proof command exactly:

```sh
cmake --build --preset default && bash -o pipefail -c "ctest --test-dir build -j --output-on-failure -R '^backend_' | tee test_after.log"
```

Result: passed. The build completed and CTest reported `100% tests passed, 0
tests failed out of 169`.

Proof log: `test_after.log`
