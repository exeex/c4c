Status: Active
Source Idea Path: ideas/open/386_rv64_object_route_same_module_byval_aggregate_call_args.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Lower Supported Same-Module Byval Aggregate Call Argument

# Current Packet

## Just Finished

Step 2: Add Focused Call-Contract Coverage completed.

Changed files:

- `src/backend/prealloc/call_plans.cpp`
- `tests/backend/bir/backend_prepare_frame_stack_call_contract_test.cpp`
- `todo.md`

Focused coverage now proves a semantic RV64 same-module byval aggregate-address
call contract independent of `src/20030914-2.c`, callee name `f`, source
register `s1`, or literal `4660`:

- arg0 is `aggregate_address` with register source and selected local
  frame-address payload facts.
- prepared `aggregate_transport=stack_copy` publishes explicit byval payload
  size/alignment, copy size/alignment, source stack offset, and chunk offsets.
- arg0 has no destination register or prepared outgoing stack argument area.
- scalar arg1 is immediate-sourced and placed in `a1`.
- GPR result is published from `a0` to the caller value register.
- missing byval ABI metadata and ambiguous local aggregate-address
  materialization both fail closed by publishing no aggregate transport.

Small prepared-contract extension made in `call_plans.cpp`: byval-copy pointer
arguments may now use the existing local frame-address source selection, and
`plan_prepared_aggregate_transport` publishes `PreparedAggregateTransportKind::StackCopy`
only when explicit byval ABI size/alignment and a unique selected payload source
are present.

## Suggested Next

Execute Step 3 from `plan.md`: lower the supported same-module RV64 byval
aggregate/address argument by consuming `aggregate_transport=stack_copy` and
the selected local frame-address payload facts from the prepared call argument
plan. Step 3 can lower from explicit prepared facts without reconstructing
payload layout from testcase names, raw BIR text, or stack-object suffixes.

## Watchouts

- Do not reopen prepared global-symbol publication; idea 384 closed that
  boundary.
- Do not treat closed byval parameter-home idea 370 as owning this callsite
  movement boundary.
- Keep aggregate `va_arg` and non-register parameter-home work routed to ideas
  371 and 374 respectively.
- Do not modify or rely on unrelated EV64 idea 385 for this lifecycle route.
- Keep Step 3 fail-closed for byval aggregate-address arguments without
  `aggregate_transport=stack_copy`, without selected payload source facts, with
  ambiguous payload materialization, or with byval metadata outside the selected
  size/alignment contract.

## Proof

Delegated proof command:

```sh
cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_call_boundary_effect_plan$'
```

Result: passed. Canonical proof log: `test_after.log`.

Additional focused sanity check:

```sh
ctest --test-dir build -j --output-on-failure -R '^backend_prepare_frame_stack_call_contract$'
```

Result: passed.
