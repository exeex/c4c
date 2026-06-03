Status: Active
Source Idea Path: ideas/open/105_prealloc_raw_global_address_identity_fallback_contract.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Implement The Structured Identity Boundary

# Current Packet

## Just Finished

Step 3 implementation completed for the raw global structured identity
boundary.

Implemented in `src/backend/prealloc/stack_layout/coordinator.cpp`:
- Replaced the broad `find_global_by_symbol` fallback with
  `find_raw_no_id_global_address_compatibility`, which only resolves module
  globals whose `Global::link_name_id == kInvalidLinkName` and whose raw
  `Global::name` matches the raw reference.
- Added named structured and compatibility helper boundaries:
  `resolve_structured_global_symbol_address`,
  `resolve_raw_no_id_global_address_compatibility`, and
  `resolve_prepared_global_symbol_address`.
- With a supplied `LinkNameId`, prepared global address construction now uses
  ID-only global lookup. Raw text is allowed only when empty or matching the
  structured spelling; drifted raw text fails closed.
- Without a supplied ID, raw spelling can publish a prepared global symbol only
  through `RawNoIdGlobalAddressCompatibility`; raw text no longer selects module
  globals that have structured `Global::link_name_id`.
- Explicit/GEP-derived `MemoryAddress::GlobalSymbol` routes now use the same
  structured/compatibility helper as ordinary global load/store construction.
  Missing `base_link_name_id` for a structured global fails closed instead of
  recovering through `base_name`.
- String-constant prepared addresses retain their existing display/text path.
  Direct/GOT/TLS global materialization code was not changed and remains
  fail-closed on missing `Value::pointer_symbol_link_name_id`.

No `addressing.hpp` schema marker was needed because the retained compatibility
class is enforced at the coordinator helper boundary before
`PreparedAddress::symbol_name` is published.

## Suggested Next

Execute `plan.md` Step 4: add focused proof for ordinary global loads/stores,
explicit/GEP-derived `MemoryAddress::GlobalSymbol`, direct global
materialization, and initializer-derived pointer-symbol materialization.

## Watchouts

- Do not make raw symbol spelling the primary identity path when structured
  `LinkNameId` exists.
- Keep final target relocation selection and TLS lowering out of scope.
- Existing direct global materialization already fails closed on missing
  `pointer_symbol_link_name_id`; Step 4 should prove rather than weaken that
  path.
- Step 4 should include negative proof that raw/no-id references do not resolve
  structured globals, for both ordinary global access and explicit/GEP-derived
  `MemoryAddress::GlobalSymbol` access.
- If the raw/no-id compatibility path remains covered, prove it with a raw/no-id
  module global, not a structured global with matching raw text.

## Proof

Passed. Ran:
`cmake --build --preset default 2>&1 | tee test_after.log; test ${PIPESTATUS[0]} -eq 0`

Proof log: `test_after.log`.
