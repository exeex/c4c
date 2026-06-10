Status: Active
Source Idea Path: ideas/open/153_bir_select_chain_direct_global_identity.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Prove Prepared/BIR Select-Chain Equivalence

# Current Packet

## Just Finished

Step 4 of `plan.md` is complete. Broadened
`backend_prepared_lookup_helper_test` prepared/BIR select-chain equivalence
proof without switching any consumers.

- Added a local equivalence helper that compares prepared select-chain source,
  direct-global dependency, and scalar materialization answers against
  `find_bir_select_chain_source_producer(...)`,
  `find_bir_select_chain_direct_global_dependency(...)`,
  `find_bir_select_chain_scalar_materialization_eligibility(...)`, and
  `find_bir_select_chain_identity(...)`.
- Positive coverage now includes select root, direct `LoadGlobal` root, local
  no-dependency scalar materialization, and nested non-select cast/binary
  direct-global paths.
- Negative coverage now includes before-boundary rejection, missing direct
  dependency, missing root/name mismatch, and mismatched root type.
- `backend_aarch64_instruction_dispatch_test` was included in proof but did not
  need edits; no AArch64/prealloc consumer implementation was switched.

## Suggested Next

Execute Step 5 of `plan.md`: switch one route-local consumer to the proven BIR
select-chain identity facts while keeping prepared queries available as the
oracle/fallback surface.

## Watchouts

- Keep `review/phase_a_steps_1_4_route_review.md` untouched.
- Keep prepared select-chain queries available as the oracle/fallback during
  the first consumer switch.
- Do not import target materialization cost, register availability,
  publication routing, call ABI behavior, or final AArch64 move/branch choices
  into the BIR select-chain relationship.
- AArch64/prealloc consumers remain on prepared authority after this packet;
  the next packet should switch only one narrow identity-read consumer.
- The new direct-global dependency record reports the matched `LoadGlobalInst`
  and its dependency instruction index; root instruction index remains on
  `BirSelectChainIdentity`.

## Proof

Delegated proof command:

```bash
set -o pipefail && cmake --build build --target backend_prepared_lookup_helper_test backend_aarch64_instruction_dispatch_test && ctest --test-dir build -R '^(backend_prepared_lookup_helper|backend_aarch64_instruction_dispatch)$' --output-on-failure | tee test_after.log
```

Result: passed, `2/2` tests green:

- `backend_prepared_lookup_helper`
- `backend_aarch64_instruction_dispatch`

Proof log path: `test_after.log`.
