Status: Active
Source Idea Path: ideas/open/522_bir_route1_scalar_producer_body_extraction.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Select Route1 Body Boundary

# Current Packet

## Just Finished

Step 2 - Select Route1 Body Boundary completed as a decision-only packet.

Selected boundary:
- Move route1 scalar producer bodies to new
  `src/backend/bir/bir_route1.cpp`.
- Preserve all public route1 declarations, records, and public route1 types in
  `src/backend/bir/bir.hpp`.
- Do not add a new private helper boundary for this slice; the Step 1 audit
  found no route1 dependency on `bir_private.hpp`, `route_block_matches`, or
  route2/route4/route5/route6/route7/route8 implementation internals.

Included body boundary for Step 3:
- Private route1-only helpers:
  `produced_value_for_route1_producer` and
  `route1_evaluate_same_block_integer_constant_impl`.
- Public route1 body definitions:
  `route1_source_value_identity`,
  `route1_immediate_integer_constant`, `route1_producer_kind`,
  `route1_produced_value`, `route1_producer_instruction_identity`,
  `route1_producer_record`, `route1_build_producer_index`,
  `route1_find_same_block_scalar_producer`,
  `route1_find_materialization_availability`, and both
  `route1_evaluate_same_block_integer_constant` wrapper definitions.

Excluded boundary:
- Public route1 declarations, route1 structs/types, `Value`, `Inst`, `Block`,
  `Function`, `Module`, `MemoryAddress`, and broad BIR declarations stay in
  `bir.hpp` or their current owners.
- Route2, route4, route5, route6, route7, route8, and route-index facade bodies
  stay in their current owners and continue to consume route1 through the
  existing public surface.
- Comparison-only anonymous helpers remain out of the route1 extraction:
  `produced_value_for_comparison_producer`,
  `comparison_producer_kind_for_inst`, `is_comparison_binary_opcode`,
  `find_unique_comparison_producer`, and
  `evaluate_comparison_integer_constant`.

Rejected alternatives:
- Preserve route1 bodies in `bir.cpp`: rejected because the Step 1 audit found
  the route1 bodies separable, and the active idea/destination map intend a
  focused route1 implementation owner.
- Move declarations or split public headers: rejected because broad downstream
  consumers across routes and non-BIR code rely on the existing `bir.hpp`
  surface.
- Move downstream route consumers with route1: rejected as route drift and
  outside this behavior-preserving body extraction.
- Introduce a new shared/private route helper boundary: rejected for Step 3
  because the selected route1 bodies can preserve helper visibility inside the
  new TU without depending on private route helpers.

## Suggested Next

Execute Step 3 from `plan.md`: create `src/backend/bir/bir_route1.cpp`, move
only the included route1 bodies and private route1-only helpers listed above,
preserve route1 declarations in `bir.hpp`, and add only required build metadata
for the new TU and any direct-source BIR tests. Do not change producer identity,
integer constant evaluation, source/value matching, downstream route semantics,
tests, expectations, or unsupported markers.

## Watchouts

Step 3 will need build metadata for `bir_route1.cpp` and any direct-source BIR
tests that manually compile BIR sources. `route1_evaluate_same_block_integer_constant`
has public consumers outside `bir.cpp`, so keep the wrappers and declarations
public. Keep the comparison-only helpers in `bir.cpp`; they are not part of the
route1 scalar producer boundary.

## Proof

No build or ctest proof required for this decision-only packet.

Validation run:
- `git diff --check -- todo.md`
