# Current Packet

Status: Active
Source Idea Path: ideas/open/80_aarch64_dispatch_publication_owner_relocation.md
Source Plan Path: plan.md
Current Step ID: Step 4
Current Step Title: Contract obsolete dispatch publication surfaces

## Just Finished

Completed Step 4 from `plan.md`: contracted obsolete
`dispatch_publication.*` surface dependencies without moving or deleting the
remaining shared helpers. The file pair now remains focused on shared scalar
helper declarations plus current-block publication authority: prepared value
home lookup, current-block entry publication detection/register publication,
recording current-block entry publication registers, and the branch-condition
publication wrapper that wires those helpers into comparison lowering.

Removed stale includes from `dispatch_publication.cpp` that were made obsolete
by prior owner relocations, and removed the unused `<vector>` include from
`dispatch_publication.hpp`. No declarations were removed because the remaining
public surface still has live users across dispatch, ALU/cast/memory/global,
value-materialization, producer, and comparison paths.

Changed files:

- `src/backend/mir/aarch64/codegen/dispatch_publication.cpp`
- `src/backend/mir/aarch64/codegen/dispatch_publication.hpp`
- `todo.md`

## Suggested Next

Proceed to Step 5 with supervisor-selected validation of the contracted
dispatch-publication owner-relocation slice.

## Watchouts

- `dispatch_publication.hpp` still intentionally exposes generic
  scalar/register helpers because they remain shared by multiple existing
  lowering owners and the packet explicitly excluded moving that family.
- `dispatch_publication.cpp` still depends on comparison, dispatch producer,
  value-publication, prepared-home materialization, memory frame-pointer policy,
  MIR query, prepared lookup, ABI, and ALU headers for the retained
  current-block publication/branch-fusion authority.
- The untracked `review/` artifact remains ignored for implementation state.

## Proof

Ran the supervisor-selected proof command exactly:

```sh
cmake --build --preset default && bash -o pipefail -c "ctest --test-dir build -j --output-on-failure -R '^backend_' | tee test_after.log"
```

Result: passed. The build completed and CTest reported `100% tests passed, 0
tests failed out of 169`.

Proof log: `test_after.log`
