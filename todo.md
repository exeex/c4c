Status: Active
Source Idea Path: ideas/open/153_bir_select_chain_direct_global_identity.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Switch One Route-Local Consumer If Proven

# Current Packet

## Just Finished

Step 5 of `plan.md` is complete. Switched the route-local
`select_chain_contains_direct_global_load(...)` identity read in
`dispatch_producers.cpp` from prepared select-chain dependency lookup to
`mir::find_bir_select_chain_direct_global_dependency(...)`.

- The helper now constructs a `mir::BirSelectChainIdentityRequest` from the
  current BIR block, BIR block label, root value, and before-instruction
  boundary.
- Prepared select-chain dependency queries remain available elsewhere as
  oracle/fallback authority; broader materialization and call-routing consumers
  were not switched.
- `backend_aarch64_instruction_dispatch_test` was included in proof but did not
  need edits.

## Suggested Next

Supervisor should review/commit the Step 5 slice or delegate the next
route-local consumer only after confirming this first switch is accepted.

## Watchouts

- Keep `review/phase_a_steps_1_4_route_review.md` untouched.
- Keep prepared select-chain queries available as the oracle/fallback while
  evaluating the first consumer switch.
- Do not import target materialization cost, register availability,
  publication routing, call ABI behavior, or final AArch64 move/branch choices
  into the BIR select-chain relationship.
- AArch64/prealloc materialization and call-routing consumers remain on
  prepared authority after this packet.

## Proof

Delegated proof command:

```bash
set -o pipefail && cmake --build build --target backend_prepared_lookup_helper_test backend_aarch64_instruction_dispatch_test && ctest --test-dir build -R '^(backend_prepared_lookup_helper|backend_aarch64_instruction_dispatch)$' --output-on-failure | tee test_after.log
```

Result: passed, `2/2` tests green:

- `backend_prepared_lookup_helper`
- `backend_aarch64_instruction_dispatch`

Proof log path: `test_after.log`.
