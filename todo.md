Status: Active
Source Idea Path: ideas/open/65_aarch64_target_owner_relocation.md
Source Plan Path: plan.md
Current Step ID: Step 5
Current Step Title: Retire Dispatch-Family Adapter Surface

# Current Packet

## Just Finished

Step 5 completed: retired the remaining transitive memory-helper exposure from
the broad dispatch-publication header and recorded the remaining justified
dispatch-family adapter surfaces.

Changes:
- Removed `memory.hpp` from `dispatch_publication.hpp`; moved memory/address
  helper consumers now include the precise memory owner directly.
- Added direct `memory.hpp` includes to `cast_ops.cpp`,
  `dispatch_edge_copies.cpp`, `globals.cpp`,
  `prepared_value_home_materialization.cpp`, and `variadic.cpp`.
- Confirmed Step 2 global helpers are exposed from `globals.hpp`, Step 3
  memory/address helpers from `memory.hpp`, and Step 4 alias helpers from
  `alu.hpp`.
- Added no forwarding wrappers and did not move helper logic into
  `dispatch.cpp`.

Remaining justified adapter surface:
- `dispatch_producers.hpp` still owns producer/source-query routing helpers:
  same-block select producer discovery, select-chain global-load discovery,
  producer instruction-index lookup, and current-block join prepared-query
  routing. These are semantic/route adapters, not target spelling helpers.
- `dispatch_value_materialization.hpp` still exposes
  `emit_value_publication_to_register`; it remains a broad recursive
  target-local value-publication adapter used by several owners and should not
  be split through forwarding-only cleanup.
- `dispatch_publication.hpp` still exposes target-local publication,
  prepared-home, retargeting, fixed-formal/local-slot, block-entry, and
  branch-fusion publication hooks. Those are still live route/publication
  adapters until a precise publication owner exists.

## Suggested Next

Step 6 should prove target-owner preservation and closure readiness. Suggested
packet: run the supervisor-selected final AArch64/backend proof, repeat
route-quality scans for the relocated global/memory/ALU helper surfaces,
confirm `dispatch.cpp` did not accumulate helper logic, and decide whether the
active plan is ready for plan-owner closure review.

## Watchouts

No `publication.*` owner existed in this checkout, so remaining
dispatch-publication declarations are justified rather than force-moved into a
new source during surface cleanup. Do not broaden Step 6 into behavior changes,
dispatch sequencing, before-return ordering, prepared-memory retry decisions,
current-block join query routing, or shared semantic authority.

The global relocation helper in `dispatch_publication.*` and the private
global-load publication helper in `dispatch_value_materialization.cpp` remain
deferred.

## Proof

Proof passed:
`(cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_aarch64_|backend_codegen_route_aarch64_)') > test_after.log 2>&1`

`test_after.log` reports 40/40 tests passed. `git diff --check` passed.
Route-quality scans confirmed direct `memory.hpp` includes for moved
memory/address-helper consumers and confirmed relocated Step 2-4 helper
declarations live in `globals.hpp`, `memory.hpp`, and `alu.hpp`, not in broad
dispatch-family headers.
