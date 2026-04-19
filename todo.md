# Execution State

Status: Active
Source Idea Path: ideas/open/64_shared_text_identity_and_semantic_name_table_refactor.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Migrate The First Prepared Identity Surfaces
Plan Review Counter: 5 / 10
# Current Packet

## Just Finished

Completed another `plan.md` Step 3 packet by splitting
`PreparedStackObject` identity into typed semantic carriers instead of one raw
`source_name` string: local-slot-backed objects now publish `SlotNameId`,
byval/sret parameter-backed objects now publish `ValueNameId`, stack-layout
and liveness helpers now consume those typed ids through `PreparedNameTables`,
and the stack-layout backend tests now assert object identity through the
semantic-name tables rather than by reading a raw public string field.

## Suggested Next

Continue `plan.md` Step 3 with the next prepared/public symbolic surface that
still accepts raw spellings as part of its contract, likely the convenience
lookup and classification helpers in `prealloc.hpp` that still take
`std::string_view` names for blocks/values/slots instead of typed semantic ids
now that stack-layout objects themselves no longer publish an untyped name.

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
kept `backend_prepare_stack_layout`, `backend_prepare_liveness`, and the x86
handoff boundary coverage green with semantic stack-object slot/value ids in
place, and the run reproduced only the same four known failing tests already
present in `test_before.log`, so this packet did not introduce a new subset
regression.
