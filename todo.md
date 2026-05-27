# Current Packet

Status: Active
Source Idea Path: ideas/open/56_aarch64_edge_terminator_consumer_preservation_repair.md
Source Plan Path: plan.md
Current Step ID: Step 1
Current Step Title: Establish edge preservation baseline

## Just Finished

Step 1 baseline established for the edge/terminator consumer preservation route.
The focused eight-test subset built cleanly and passed `7/8`; only
`c_testsuite_aarch64_backend_src_00204_c` still fails with
`[RUNTIME_MISMATCH]`.

Prepared BIR for `myprintf` shows `vaarg.join.39` begins with:
`%t45.byte_offset.i64 = bir.sext i32 %t35 to i64`,
`%t45 = bir.add ptr %t44, %t45.byte_offset.i64`,
`%t49.phi.sel0 = bir.select ... ptr %t45, %t47`, and
`%t49 = bir.select ... ptr %t47, %t49.phi.sel0`.

The late consumer move is prepared by
`src/backend/prealloc/regalloc/consumer_moves.cpp::append_consumer_move_resolution`
through `append_for_instruction`, then bucketed by
`src/backend/prealloc/regalloc.cpp::append_prepared_move_bundle` into
`PreparedValueLocationFunction::move_bundles`. The concrete bundle is
`phase=before_instruction authority=none block_index=16 instruction_index=0`
with `move from_value_id=2085 (%t35) to_value_id=2090
(%t45.byte_offset.i64) destination_storage=stack_slot
reason=consumer_register_to_stack`. At that point `%t35`'s prepared home is
`kind=register reg=x13`, while `%t45.byte_offset.i64` is stack slot `#2933`
at offset `976`.

The predecessor edge publication is owned by prepared control-flow
`join_transfer`/`edge_transfer` records plus the out-of-SSA parallel-copy move
bundles. `vaarg.join.39` has an authoritative select-materialization
`join_transfer` for `%t49`: `vaarg.reg.38 -> vaarg.join.39 incoming=%t45
destination=%t49` and `vaarg.stack.36 -> vaarg.join.39 incoming=%t47
destination=%t49`. The concrete clobbering bundles are
`phase=block_entry authority=out_of_ssa_parallel_copy block_index=14
source_parallel_copy=vaarg.reg.38 -> vaarg.join.39` with
`from_value_id=2091 (%t45) to_value_id=2094 (%t49)`, and `block_index=15
source_parallel_copy=vaarg.stack.36 -> vaarg.join.39` with
`from_value_id=2092 (%t47) to_value_id=2094 (%t49)`. Both target `%t49`'s
prepared home `kind=register reg=x13`, so predecessor terminator publication
can make `x13` authoritative for `%t49` before the join-block
`%t35 -> %t45.byte_offset.i64` consumer move runs.

The edge-publication lookup owner is
`src/backend/prealloc/prepared_lookups.cpp::make_prepared_edge_publication_lookups`,
which builds `PreparedEdgePublicationLookups::publications` from
`PreparedControlFlowFunction::join_transfers`, finds the matching
`PreparedMoveBundle` through `find_edge_publication_move`, and records the
parallel-copy step with `find_edge_publication_parallel_copy_step`.
`src/backend/prealloc/publication_plans.cpp` owns the scalar/store-source
publication planning helpers that consume those prepared edge-publication and
source-producer facts.

The emitted AArch64 symptom matches the prepared ordering: after the predecessor
path has `%t49` in `x13`, `.LBB154_28` emits `mov w9, w13; sxtw x9, w9` for
the `%t35 -> %t45.byte_offset.i64` cast, then uses `x13` as the byte-copy
source. That is the physical-register clobber: the same `x13` home is used for
`%t35`, `%t47`, and `%t49`, and `%t49` wins before the join consumer runs.

## Suggested Next

Proceed to Step 2: define the preservation contract. Inspect whether the
`append_consumer_move_resolution` consumer move should be placed on predecessor
terminators/edge bundles when a successor `before_instruction` bundle would run
after an out-of-SSA edge publication that overwrites the source register, or
whether `make_prepared_edge_publication_lookups`/publication planning should
publish an explicit predecessor-side preservation fact for codegen to consume.

## Watchouts

Do not edit or revert the existing dirty `memory.cpp` and
`dispatch_edge_copies.cpp` context during lifecycle-only work. They remain
incomplete implementation context until a supervisor accepts, splits, or
reroutes them.

Do not reload `%lv.ap.24` in the join block. Do not special-case `00204`,
`myprintf`, `%t35`, `%t45`, `%t49`, `vaarg.join.39`, or `x13`.

The baseline facts were collected from the current dirty worktree, including
the existing unaccepted `memory.cpp` and `dispatch_edge_copies.cpp` context.
Do not treat this as an implementation acceptance signal; it is only the Step 1
ordering baseline.

## Proof

Focused proof command run for this packet:

`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_codegen_route_aarch64_pointer_select_aggregate_byte_copy|backend_codegen_route_aarch64_variadic_aggregate_overflow_byte_copy|backend_codegen_route_aarch64_alu_unpublished_load_local_(after_call|call_boundary)|c_testsuite_aarch64_backend_src_00164_c|c_testsuite_aarch64_backend_src_00176_c|c_testsuite_aarch64_backend_src_00181_c|c_testsuite_aarch64_backend_src_00204_c)$'`

Result: `7/8` passed. The only failing test was
`c_testsuite_aarch64_backend_src_00204_c` with `[RUNTIME_MISMATCH]`.

Proof log: `test_after.log`.
