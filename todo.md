# Current Packet

Status: Active
Source Idea Path: ideas/open/56_aarch64_edge_terminator_consumer_preservation_repair.md
Source Plan Path: plan.md
Current Step ID: Step 2
Current Step Title: Define the preservation contract

## Just Finished

Step 2 contract selected: predecessor-side preservation belongs in
`src/backend/prealloc/regalloc/consumer_moves.cpp`, not in edge-publication
planning and not as a shared lookup-first rule.

The smallest contract is: when `append_consumer_move_resolution` is about to
record a successor `before_instruction` consumer move from a named operand to
the named result storage, it must also consider each predecessor edge into that
successor. If an existing prepared edge publication for that predecessor and
successor writes a destination home that shares the operand source's prepared
register identity, then the consumer dependency is too late in the successor
block. The planner must create an edge-scoped preservation move for the same
semantic source and consumer destination on the predecessor edge before the
clobbering publication is allowed to make the shared register authoritative for
another value.

The owner is consumer-move planning because the thing being preserved is the
successor instruction's operand-to-result consumer dependency. Edge-publication
planning already owns the clobbering transfer facts, but it does not own which
successor instruction operands must remain available after block-entry
publication. `prepared_lookups.cpp` should remain an index/query surface for
publication facts and may gain a helper only if implementation needs a clean
way to ask "which edge publications into this successor clobber this source
home"; it should not become the rule owner. `publication_plans.cpp` remains out
of scope for this contract because scalar/store publication plans consume
prepared facts after placement rather than deciding successor consumer
scheduling.

The placement rule must be semantic and prepared-ID based: source and
destination are `PreparedValueId`/`ValueNameId` plus `PreparedValueHome`
identity, predecessor/successor are `BlockLabelId`, and clobber detection is
based on prepared home/register identity. It must not key on raw temporary
spellings, target testcase names, function names, block labels, physical
register spellings alone, or the current `00204` temporary names.

Exact implementation-owner files for the next packet:
`src/backend/prealloc/regalloc/consumer_moves.cpp`,
`src/backend/prealloc/regalloc.hpp`, and
`src/backend/prealloc/regalloc.cpp`. Use
`src/backend/prealloc/prepared_lookups.cpp` and
`src/backend/prealloc/prepared_lookups.hpp` only if the implementation needs a
small reusable clobber query instead of a local scan. Do not start in
`src/backend/prealloc/publication_plans.cpp`; only touch
`src/backend/mir/aarch64/codegen/dispatch_edge_copies.cpp` if the prepared
bundle shape proves codegen cannot emit the new edge-scoped preservation before
the edge publication.

## Suggested Next

Implement the Step 3 prepared-planning slice: extend consumer-move planning to
emit edge-scoped predecessor preservation for successor consumer moves whose
source home is clobbered by an incoming block-entry edge publication. If needed,
add a distinct prepared move authority/phase classification so the move is a
block-entry edge preservation without pretending to be the out-of-SSA
publication itself.

## Watchouts

Do not edit or revert the existing dirty `memory.cpp` and
`dispatch_edge_copies.cpp` context during lifecycle-only work. They remain
incomplete implementation context until a supervisor accepts, splits, or
reroutes them.

Do not reload `%lv.ap.24` in the join block. Do not special-case `00204`,
`myprintf`, `%t35`, `%t45`, `%t49`, `vaarg.join.39`, or `x13`.

The current phase model classifies `PreparedMoveDestinationKind::Value` as
`block_entry` only for `PreparedMoveAuthorityKind::OutOfSsaParallelCopy`.
Implementation must either add a clear edge-consumer-preservation authority (or
equivalent explicit phase rule) or prove that reusing the out-of-SSA authority
does not confuse publication lookup, block-entry publication collection, or
edge-copy filtering. Avoid value-name-only/testcase-shaped placement rules.

## Proof

Focused proof command run for this packet:

`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_codegen_route_aarch64_pointer_select_aggregate_byte_copy|backend_codegen_route_aarch64_variadic_aggregate_overflow_byte_copy|backend_codegen_route_aarch64_alu_unpublished_load_local_(after_call|call_boundary)|c_testsuite_aarch64_backend_src_00164_c|c_testsuite_aarch64_backend_src_00176_c|c_testsuite_aarch64_backend_src_00181_c|c_testsuite_aarch64_backend_src_00204_c)$'`

Result after this Step 2 contract-only packet: build passed, `7/8` focused
tests passed, and only `c_testsuite_aarch64_backend_src_00204_c` failed with
`[RUNTIME_MISMATCH]`. This matches the expected unresolved baseline because no
implementation files were changed.

Proof log: `test_after.log`.
