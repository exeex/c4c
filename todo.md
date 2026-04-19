# Execution State

Status: Active
Source Idea Path: ideas/open/64_shared_text_identity_and_semantic_name_table_refactor.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Migrate The First Prepared Identity Surfaces
Plan Review Counter: 6 / 10
# Current Packet

## Just Finished

Completed another `plan.md` Step 3 packet by moving the prepared
addressing/control-flow lookup entry points that were already backed by typed
storage off raw function/block spellings and onto semantic ids: the public
lookup helpers in `prealloc.hpp` now enter through `FunctionNameId` and
`BlockLabelId`, x86 prepared-module consumers translate BIR spellings to typed
ids only at the boundary, and the backend tests now look up prepared
addressing/branch metadata through those semantic ids instead of through
string-keyed helper overloads.

## Suggested Next

Continue `plan.md` Step 3 with the remaining prepared/public helper surfaces
that still accept raw symbolic spellings inside `prealloc.hpp`, especially the
branch/join classification helpers that still key by string block/value names
because their underlying prepared records have not yet been fully lifted to
typed semantic ids.

## Watchouts

- Keep this runbook on idea 64 scope only. Do not pull in idea 62 CFG or idea
  63 phi algorithm work while introducing semantic ids.
- `PreparedStackObject` no longer has a single spelling field. Future direct
  consumers must choose the correct semantic domain: slot-backed objects use
  `slot_name`, parameter/value-backed objects use `value_name`, and mixed
  helper code should resolve spellings through `PreparedNameTables` only when a
  test or fallback path truly needs bytes.
- Stack-layout hint passes such as alloca/copy coalescing and regalloc
  widening still reason over BIR local slots. Keep later migrations from
  smuggling params into those local-slot-only helpers just because both domains
  now live in `PreparedStackObject`.
- The delegated backend subset still has the same four pre-existing failures
  from baseline:
  `backend_codegen_route_x86_64_variadic_double_bytes_observe_semantic_bir`,
  `backend_codegen_route_x86_64_variadic_pair_second_observe_semantic_bir`,
  `backend_codegen_route_x86_64_local_direct_dynamic_member_array_store_observe_semantic_bir`,
  and
  `backend_codegen_route_x86_64_local_direct_dynamic_member_array_load_observe_semantic_bir`.

## Proof

Ran `cmake --build --preset default && ctest --test-dir build -j
--output-on-failure -R '^backend_' > test_after.log 2>&1` and captured the
build/test output in `test_after.log`. The build completed, the backend subset
kept the prepared stack-layout/control-flow tests and x86 handoff boundary
coverage green with semantic function/block-id lookups in place, and
`test_after.log` matched `test_before.log` exactly, reproducing only the same
four known failing tests already called out above, so this packet did not
introduce a new subset regression.
