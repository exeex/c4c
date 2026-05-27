# Current Packet

Status: Active
Source Idea Path: ideas/open/55_aarch64_scalar_cast_alu_publication_prepared_authority_repair.md
Source Plan Path: plan.md
Current Step ID: Step 3
Current Step Title: Repair prepared source preservation or placement

## Just Finished

Step 3 precisely classified the prepared placement gap without code changes.
Prepared BIR already records the semantic consumer move
`%t35 -> %t45.byte_offset.i64` as a `consumer_register_to_stack` move, but it
is scheduled as a before-instruction move at `vaarg.join.39` instruction 1.
That is too late: the predecessor edge/publication path has already made `x13`
authoritative for `%t49`, while `%t35` was originally loaded from `%lv.ap.24`
before the va_list gp-offset mutation.

The exact missing prepared fact is a predecessor-side preservation/placement
point for the consumer move before the out-of-SSA edge publication overwrites
the shared physical register. Current prepared data can describe:

- `%t35` home: register `x13`
- `%t49` home: register `x13`
- `%t35 -> %t45.byte_offset.i64`: before-instruction consumer move at the join
- `%t45` / `%t47 -> %t49`: out-of-SSA edge publication at predecessor
  terminators

It cannot currently express "spill or materialize this later join consumer
before the predecessor terminator edge copy clobbers the source register."
Adding that by pretending the join-time consumer move still has a valid `x13`
source would reproduce the stale-source bug. Reloading `%lv.ap.24` at the join
is also invalid because the va_list field has already been updated.

## Suggested Next

Plan review or split is needed before more implementation. The next owner
should define a prepared terminator/edge-preservation contract that can place
ordinary consumer preservation before out-of-SSA edge publication reuses the
source register, or explicitly move this case under the edge publication owner.

## Watchouts

Do not reload `%lv.ap.24` at the join. The va_list field has already been
updated, so that would be a stale-source fix in the opposite direction.

Do not special-case `%t35`, `%t45`, `%t49`, `vaarg.join.39`, `myprintf`, or
`00204`. The general rule is that scalar cast/ALU publication must not trust an
emitted register mapping after prepared block-entry or edge publication has
made the same physical register authoritative for a different value.

Do not accept or commit the dirty `memory.cpp` and `dispatch_edge_copies.cpp`
changes as part of this lifecycle reset. They remain incomplete context that
removed the original `00204` segfault and bad cursor reload, but focused proof
still fails `00204`.

If the prepared preservation/placement owner is outside idea 55's bounded
surface, stop for a lifecycle split instead of broadening the implementation
packet.

The current owned files can classify the issue, but a complete semantic repair
appears to require a new edge/terminator preservation phase or an extension of
out-of-SSA edge publication authority. A cast-only consumer patch is still
unsafe, because `cast_ops.cpp` has no valid current `%t35` source after entry.

## Proof

Proof command:

`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_codegen_route_aarch64_pointer_select_aggregate_byte_copy|backend_codegen_route_aarch64_variadic_aggregate_overflow_byte_copy|backend_codegen_route_aarch64_alu_unpublished_load_local_(after_call|call_boundary)|c_testsuite_aarch64_backend_src_00164_c|c_testsuite_aarch64_backend_src_00176_c|c_testsuite_aarch64_backend_src_00181_c|c_testsuite_aarch64_backend_src_00204_c)$'`

Result: `7/8` passed. The only failing test is
`c_testsuite_aarch64_backend_src_00204_c`, still failing as
`[RUNTIME_MISMATCH]`. This proof is sufficient for the classification packet
but not an acceptance proof for code progress.

Proof log path: `test_after.log`.
