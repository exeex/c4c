Status: Active
Source Idea Path: ideas/open/153_bir_select_chain_direct_global_identity.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Define BIR Select-Chain Identity Records

# Current Packet

## Just Finished

Step 2 of `plan.md` is complete. Added target-neutral BIR select-chain identity
record shapes near the existing MIR query surface:

- `BirSelectChainIdentityRequest` keys the relationship by BIR block, optional
  display block label, root value/value name/type, and before-instruction
  boundary.
- `BirSelectChainDirectGlobalDependency` represents direct `LoadGlobalInst`
  dependency presence plus the dependency instruction pointer/index, without
  target materialization cost, register state, publication policy, call ABI, or
  final emission fields.
- `BirSelectChainIdentity` represents the root producer, root value/name,
  root-is-select, root instruction index, direct-global dependency identity,
  and scalar materialization eligibility.
- `src/backend/mir/query.cpp` was inspected through `c4c-clang-tools`; the
  existing same-block producer/source identity surface was reused and no
  consumer implementation was switched.
- `backend_prepared_lookup_helper_test` now proves the record shape can encode
  the prepared oracle's select-root direct-global dependency, direct
  `LoadGlobal` root, scalar materialization availability, and missing-root
  fail-closed case.

## Suggested Next

Execute Step 3 of `plan.md`: add BIR-owned select-chain query functions that
populate these records from BIR instructions and same-block producer/source
facts, while keeping prepared queries as the comparison oracle and leaving
consumers unchanged.

## Watchouts

- Keep `review/phase_a_steps_1_4_route_review.md` untouched.
- The new records are schema only; Step 3 still needs production query APIs
  that traverse BIR select operands and direct-global dependencies.
- Keep prepared select-chain queries available as the oracle until
  prepared/BIR equivalence is proven.
- Do not import target materialization cost, register availability,
  publication routing, call ABI behavior, or final AArch64 move/branch choices
  into the BIR select-chain relationship.
- AArch64/prealloc consumers remain on prepared authority after this packet.

## Proof

Delegated proof command:

```bash
set -o pipefail && cmake --build build --target backend_prepared_lookup_helper_test backend_aarch64_instruction_dispatch_test && ctest --test-dir build -R '^(backend_prepared_lookup_helper|backend_aarch64_instruction_dispatch)$' --output-on-failure | tee test_after.log
```

Result: passed, `2/2` tests green:

- `backend_prepared_lookup_helper`
- `backend_aarch64_instruction_dispatch`

Proof log path: `test_after.log`.
