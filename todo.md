Status: Active
Source Idea Path: ideas/open/154_bir_memory_access_identity.md
Source Plan Path: plan.md
Current Step ID: 6
Current Step Title: Acceptance Review And Close Payload

# Current Packet

## Just Finished

Step 6 recorded close-ready evidence for active idea 154 after the Step 5
consumer switch.

The accepted slice establishes a BIR memory vocabulary and query surface for
BIR-representable semantic parity:

- BIR memory access identity now names the semantic address expression and
  access kind for direct memory operations.
- Same-block global-load identity can be queried from BIR-representable
  global-symbol memory addresses.
- Same-block load-local source identity can be queried from BIR-representable
  local-slot memory addresses.
- Prepared lookup helpers remain available as the compatibility oracle and
  fallback for target/layout/addressing facts that are not BIR-representable.

Equivalence for this idea means parity for memory facts expressible in BIR, not
byte-for-byte replacement of prepared backend addressing policy. Stack-layout
range positives are intentionally preserved as prepared-only
fallback/oracle behavior because they depend on target layout and range
analysis that does not belong in BIR memory identity state.

The Step 5 accepted consumer switch is limited to one scalar global-load
publication path. It asks the BIR same-block global-load query first when the
BIR instruction carries a representable global memory address, then keeps the
prepared lookup path for legacy or non-BIR-representable positives. Prepared
fallback/oracle behavior therefore remains available after the switch.

No rejected target-layout, stack-layout range, prepared addressing,
relocation/GOT/TLS, offset-formation, or target operand state entered BIR.
The BIR side carries only BIR-representable memory address identity facts.

## Suggested Next

Next executable packet candidate: supervisor lifecycle close review for active
idea 154 using this close payload and the accepted final proof log.

## Watchouts

- Reviewer report `review/154_memory_access_step3_route_review.md` found no
  overfit or policy leakage for the route under review.
- The plan boundary rewrite at `50d5caaa9` made the accepted equivalence target
  BIR-representable semantic parity, with prepared stack-layout range positives
  retained as explicit fallback/oracle behavior.
- Closure should not claim migration of FP materialization, current global load
  materialization, local-slot/load-local consumers, layout-dependent paths,
  `globals.cpp`, `alu.cpp`, or `calls.cpp`; Step 5 switched only the scalar
  global-load publication consumer.

## Proof

`cmake --build build --target backend_aarch64_prepared_memory_operand_records_test backend_store_source_publication_plan_test backend_prepared_lookup_helper_test backend_aarch64_instruction_dispatch_test && ctest --test-dir build --output-on-failure -R '^(backend_aarch64_prepared_memory_operand_records|backend_store_source_publication_plan|backend_prepared_lookup_helper|backend_aarch64_instruction_dispatch)$' > test_after.log 2>&1`

No new test run was required for this todo-only Step 6 packet.

Accepted final proof is recorded in `test_before.log` for the matched command
above. Result: all four selected tests passed:
`backend_store_source_publication_plan`,
`backend_aarch64_prepared_memory_operand_records`,
`backend_aarch64_instruction_dispatch`, and
`backend_prepared_lookup_helper`.
