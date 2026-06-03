# Prealloc Raw Global Address Identity Fallback Contract

## Goal

Eliminate, constrain, or explicitly document raw global spelling fallback in
prealloc prepared address and materialization paths when structured
`LinkNameId` identity is absent.

## Why This Exists

The memory boundary audit classified structured global handling as correct:
BIR owns global identity, extent, initializer facts, TLS/constant/external
flags, and target-neutral materialization policy, while prealloc owns target
address preparation and relocation/TLS interpretation. The remaining gap is
the compatibility path where prepared global address or materialization
construction can fall back to raw symbol spelling when `LinkNameId` is missing.

That fallback is narrow but important for x86/RISC-V rebuild work: raw display
names must not silently become the semantic authority for global identity when
BIR can provide structured IDs.

## In Scope

- Trace raw-name fallback in global prepared address and materialization
  construction, especially around `src/backend/prealloc/addressing.hpp` and
  prealloc coordinator global-address producers.
- Decide whether missing `LinkNameId` is legitimate compatibility input,
  an assertion-worthy lowering bug, or a documented fallback for a named class
  of globals.
- Add proof that ordinary lowered globals, global GEPs, and global pointer
  initializers reach prealloc through structured identity.

## Out Of Scope

- Changing final target relocation selection or TLS model lowering.
- Reworking string constants, labels, or non-global symbol carriers unless
  they share the same missing-ID contract.
- Broad global initializer redesign.
- Rewriting unrelated pointer-carrier provenance paths.

## Acceptance Criteria

- Every raw global spelling fallback has an explicit status: removed, asserted
  unreachable for ordinary globals, or retained as a named compatibility path.
- Structured `LinkNameId` identity remains preferred whenever available.
- Proof covers ordinary global loads/stores, global address materialization,
  and at least one initializer or GEP-derived global address route.

## Reviewer Reject Signals

- The change makes raw symbol spelling the primary identity path when
  structured IDs are available.
- It only renames fallback helpers without documenting or narrowing when the
  fallback is allowed.
- It changes target relocation/TLS policy while claiming to fix an identity
  contract.
- Proof covers one named global while GEP or initializer-derived global routes
  remain unexamined.
- Tests are weakened, marked unsupported, or rewritten to hide missing
  structured identity.

## Close Note

Closed on 2026-06-03.

The source idea is complete. Every raw global spelling fallback in prepared
address and materialization construction now has explicit status. Structured
global prepared-address construction is `LinkNameId` authoritative, and raw
spelling fallback is retained only as
`RawNoIdGlobalAddressCompatibility` for legacy/raw module globals where both
the reference and resolved `Global` have no `LinkNameId`.

Structured `LinkNameId` identity remains preferred whenever available. Supplied
IDs use ID-only lookup, raw display text may be empty or matching display text
only, and drifted raw spelling with a structured ID fails closed. Missing IDs
cannot resolve module globals that have structured `Global::link_name_id`.
Direct, GOT, TLS, and indirect-callee global materialization remain
fail-closed when `Value::pointer_symbol_link_name_id` is missing.

Proof status: final backend validation passed with `169/169` backend tests, and
the close-time backend regression guard passed with `169/169` before and after,
no new failures, and no resolved failures. Coverage includes ordinary global
loads/stores, explicit and GEP-derived global address routes, global address
materialization, and initializer-derived pointer-symbol routes through
`backend_lir_to_bir_notes`.
