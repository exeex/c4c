Status: Active
Source Idea Path: ideas/open/105_prealloc_raw_global_address_identity_fallback_contract.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Final Validation And Close Readiness

# Current Packet

## Just Finished

Step 5 final validation completed for the raw global identity fallback
contract. Backend validation is green and the route is close-ready for
plan-owner closure review.

Final backend validation:
- `cmake --build --preset default` completed with no work needed.
- `ctest --test-dir build -R "^backend_" -j --output-on-failure` passed:
  169/169 backend tests, 0 failures.

Source idea acceptance coverage:
- Every raw global spelling fallback in prepared address/materialization
  construction has explicit status:
  - Structured global prepared-address construction is ID-authoritative.
  - Raw spelling fallback is retained only as
    `RawNoIdGlobalAddressCompatibility`.
  - Direct/GOT/TLS global materialization remains fail-closed on missing
    `Value::pointer_symbol_link_name_id`.
- Structured `LinkNameId` identity remains preferred whenever available:
  - supplied IDs use ID-only lookup;
  - raw text may be empty or matching display text, otherwise prepared access
    publication fails closed;
  - missing IDs cannot resolve module globals that have structured
    `Global::link_name_id`.
- Proof covers ordinary global loads/stores:
  - ID-only load publishes prepared `GlobalSymbol` facts;
  - drifted raw spelling with structured ID fails closed;
  - raw/no-id ordinary access cannot resolve a structured global;
  - retained raw/no-id compatibility is proved only with raw/no-id global
    `g.compat`.
- Proof covers explicit/GEP-derived global address routes:
  - `MemoryAddress::GlobalSymbol::base_link_name_id` publishes structured facts;
  - raw `base_name` alone cannot publish a structured global address;
  - raw/no-id `base_name` works only for raw/no-id `g.compat`.
- Proof covers global address materialization:
  - direct, GOT, TLS, and indirect-callee materializations use structured
    pointer-symbol identity;
  - raw `@name` pointer value without `pointer_symbol_link_name_id` records no
    global materialization.
- Proof covers initializer-derived routes through included
  `backend_lir_to_bir_notes` checks for scalar and aggregate pointer
  initializers carrying structured pointer-symbol IDs and rejecting missing-ID
  fallback.

Retained exception:
- `RawNoIdGlobalAddressCompatibility` remains only for legacy/raw module globals
  where both the reference and resolved `Global` have no `LinkNameId`. It is not
  allowed to select a structured global even when raw spelling matches.

## Suggested Next

Plan-owner closure review for
`ideas/open/105_prealloc_raw_global_address_identity_fallback_contract.md`.

## Watchouts

- Do not make raw symbol spelling the primary identity path when structured
  `LinkNameId` exists.
- Keep final target relocation selection and TLS lowering out of scope.
- No relocation/TLS policy changes, broad initializer redesign, string/label
  rewrites, call-plan rewrites, or pointer-carrier changes were made for this
  route.
- Close-ready status is based on green full backend validation plus focused
  Step 4 prepared-address/materialization proof.

## Proof

Passed. Ran:
`set -o pipefail; { cmake --build --preset default && ctest --test-dir build -R "^backend_" -j --output-on-failure; } 2>&1 | tee test_after.log`

Result: 169/169 backend tests passed.

Proof log: `test_after.log`.
