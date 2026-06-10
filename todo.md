Status: Active
Source Idea Path: ideas/open/153_bir_select_chain_direct_global_identity.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Add BIR-Owned Select-Chain Queries

# Current Packet

## Just Finished

Step 3 of `plan.md` is complete. Added target-neutral BIR-owned select-chain
query functions near the existing MIR query surface:

- `find_bir_select_chain_source_producer(...)` resolves the root producer from
  BIR same-block producer identity by block, root value/name/type, and
  before-instruction boundary.
- `find_bir_select_chain_direct_global_dependency(...)` walks BIR select, cast,
  and binary producer operands to find a direct same-block `LoadGlobalInst`
  dependency without consulting prepared lookup state.
- `find_bir_select_chain_scalar_materialization_eligibility(...)` reuses the
  same-block scalar materialization query for the root producer.
- `find_bir_select_chain_identity(...)` packages root producer, root value,
  root-is-select, root instruction index, direct-global dependency, and scalar
  materialization eligibility into the Step 2 records.
- `backend_prepared_lookup_helper_test` now exercises the new APIs against the
  prepared oracle for select-root direct-global dependency, direct load root,
  local no-dependency materialization, and missing-root fail-closed paths.
- No AArch64/prealloc consumer implementation was switched.

## Suggested Next

Execute Step 4 of `plan.md`: broaden prepared/BIR select-chain equivalence
proof around the new query APIs, including before-boundary, missing dependency,
type or root mismatch, and any nearby negative paths the source idea requires.

## Watchouts

- Keep `review/phase_a_steps_1_4_route_review.md` untouched.
- Keep prepared select-chain queries available as the oracle until
  prepared/BIR equivalence is proven.
- Do not import target materialization cost, register availability,
  publication routing, call ABI behavior, or final AArch64 move/branch choices
  into the BIR select-chain relationship.
- AArch64/prealloc consumers remain on prepared authority after this packet.
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
