# Current Packet

Status: Active
Source Idea Path: ideas/open/49_aarch64_dispatch_value_materialization_prepared_authority_repair.md
Source Plan Path: plan.md
Current Step ID: Step 6
Current Step Title: Route non-edge select-chain materialization through shared scalar authority

## Just Finished

Completed the first Step 5 global-load value materialization routing packet.

`dispatch_value_materialization.cpp` no longer recovers global-load labels or
GOT/direct policy through `find_load_global_target`,
`load_global_symbol_label`, `LoadGlobalInst::global_name`, or module-global
searches. Same-block `LoadGlobal` rematerialization now requires a matching
`PreparedMemoryAccess` with a prepared global-symbol base, consumes the prepared
symbol and byte offset, and selects GOT/direct emission through prepared address
policy. Prepared addressing now carries global address materialization policy
on symbol-backed accesses; the shared prepared-address helper treats an
unspecified policy as direct only for static targets and otherwise fails closed.

Route review in `review/step5-global-consumer-route-review.md` rejected the
previous Suggested Next as active Step 5 work because the remaining global-load
consumers in `globals.cpp` and `fp_value_materialization.cpp` are outside this
source idea's owned-file boundary. That follow-up is recorded separately in
`ideas/open/54_aarch64_global_value_materialization_consumer_authority_repair.md`.

## Suggested Next

Execute Step 6 in the active runbook: audit the non-edge select branch in
`dispatch_value_materialization.cpp` that calls
`emit_select_chain_value_to_register` with `prepared_named_value_id`, then
route any duplicate select-chain materialization through existing prepared
producer/scalar-publication facts or a narrowly justified shared scalar
select-chain query.

## Watchouts

`PreparedAddressMaterialization` was not sufficient for ordinary scalar
global-load values because those loads are memory accesses, not pointer address
materializations. The narrow prepared authority added here is
`PreparedAddress::global_address_materialization_policy` plus
`prepared_global_symbol_address_policy`; non-static unspecified policy remains
fail-closed. `clang-format` is not installed in this workspace, so formatting
was checked manually, with `git diff --check`, and by the build.

Do not implement `make_load_global_got_materialization_instruction` or FP
`LoadGlobal` consumer rewrites as active Step 5 work for this plan. Those are
follow-up consumer repairs under the new separate source idea unless the
supervisor explicitly switches lifecycle state.

## Proof

`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_'`.

Build succeeded; CTest passed 163/163 backend tests. Proof log:
`test_after.log`.
