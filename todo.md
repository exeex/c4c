# Current Packet

Status: Active
Source Idea Path: ideas/open/49_aarch64_dispatch_value_materialization_prepared_authority_repair.md
Source Plan Path: plan.md
Current Step ID: Step 5
Current Step Title: Route global-load materialization through shared address authority

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

## Suggested Next

Review the remaining global-load consumers outside this packet, especially
`make_load_global_got_materialization_instruction` and FP value materialization,
and decide whether they should be routed through the same prepared-address
policy helper in a follow-up Step 5 packet.

## Watchouts

`PreparedAddressMaterialization` was not sufficient for ordinary scalar
global-load values because those loads are memory accesses, not pointer address
materializations. The narrow prepared authority added here is
`PreparedAddress::global_address_materialization_policy` plus
`prepared_global_symbol_address_policy`; non-static unspecified policy remains
fail-closed. `clang-format` is not installed in this workspace, so formatting
was checked manually, with `git diff --check`, and by the build.

## Proof

`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_'`.

Build succeeded; CTest passed 163/163 backend tests. Proof log:
`test_after.log`.
