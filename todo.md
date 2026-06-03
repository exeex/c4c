Status: Active
Source Idea Path: ideas/open/105_prealloc_raw_global_address_identity_fallback_contract.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Decide The Missing-ID Contract

# Current Packet

## Just Finished

Step 2 contract decision completed for raw global missing-ID handling.

Chosen contract:
- Structured `LinkNameId` is the ordinary global identity authority. If BIR can
  publish a `LinkNameId`, prealloc prepared-address construction must consume
  that ID and must not recover semantic identity from raw spelling.
- Retain exactly one named compatibility class:
  `RawNoIdGlobalAddressCompatibility`. It is allowed only when both the
  instruction/address reference and the resolved module global are raw/no-id
  (`kInvalidLinkName`) inputs. This preserves legacy imported/raw modules that
  genuinely lack structured link-name identity.
- Missing IDs for ordinary lowered globals that resolve to a module global with
  a structured `Global::link_name_id` are assertion-worthy lowering bugs at the
  BIR producer boundary and fail-closed inputs at the prealloc boundary. Prealloc
  should not silently repair them with raw `global_name`.
- If raw spelling and structured ID disagree, structured identity wins only when
  the raw spelling is empty or matches the ID spelling; otherwise the input
  fails closed. Raw spelling is never the primary identity path when an ID
  exists.

Route decisions:
- Raw/no-id ordinary `LoadGlobalInst` / `StoreGlobalInst` prepared-address
  construction: retained only under `RawNoIdGlobalAddressCompatibility`.
  Otherwise missing `global_name_id` is a fail-closed prealloc input, and a
  normal lowering bug if the target global has a `LinkNameId`.
- Explicit/GEP-derived `MemoryAddress::GlobalSymbol` without
  `base_link_name_id`: retained only under the same
  `RawNoIdGlobalAddressCompatibility` class when the resolved global also has
  no structured link-name ID. GEP-derived addresses from normal lowered globals
  must carry `base_link_name_id`; missing ID fails closed rather than publishing
  `PreparedAddress::symbol_name` from raw `base_name`.
- `find_global_by_symbol` raw policy lookup: constrain to the named
  compatibility class. With a supplied `LinkNameId`, lookup is ID-only. Without
  a supplied ID, raw lookup may resolve only a raw/no-id module global; it must
  not use raw spelling to select a global that has structured identity. Target
  relocation/TLS policy remains unchanged after the identity is resolved.
- Direct/GOT/TLS global materialization through pointer values: preserve the
  current fail-closed requirement on `Value::pointer_symbol_link_name_id`.
  `append_direct_global_address_materialization` remains the structured
  materialization authority and must not grow a raw `@name` fallback.
- String constants and labels stay out of the global identity contract. Their
  raw display compatibility is display/text identity, not global symbol
  authority.

Implementation targets for Step 3:
- Split the current `build_direct_symbol_backed_address` raw fallback into named
  helper paths: structured global identity resolution and
  `RawNoIdGlobalAddressCompatibility`.
- Constrain `find_global_by_symbol` or replace it with helpers that make
  ID-only lookup and raw/no-id compatibility lookup auditable.
- Ensure prepared global addresses cannot publish `symbol_name` from raw
  spelling when the resolved global has a structured `LinkNameId`.
- Leave `append_direct_global_address_materialization` fail-closed on missing
  `pointer_symbol_link_name_id`.

Proof targets for Step 4:
- Ordinary global loads/stores: prove structured `global_name_id` produces
  prepared global-symbol addresses, drifted raw spelling cannot override the ID,
  and missing `global_name_id` for a structured global fails closed; add one
  explicit raw/no-id compatibility proof only if the retained compatibility path
  remains implemented.
- Explicit/GEP-derived global address route: prove
  `MemoryAddress::GlobalSymbol::base_link_name_id` is required for structured
  globals and that raw `base_name` alone cannot publish a prepared symbol for a
  structured global.
- Global address materialization: prove pointer-value direct/GOT/TLS
  materialization requires `pointer_symbol_link_name_id` and missing ID records
  no materialization.
- Initializer-derived route: prove lowered pointer initializers / initializer
  elements carry structured pointer-symbol IDs into prealloc materialization,
  and raw initializer spelling does not become a prealloc global materialization
  authority.

## Suggested Next

Execute `plan.md` Step 3: implement the structured identity boundary by
constraining prepared-address raw fallback to
`RawNoIdGlobalAddressCompatibility`, fail-closing missing IDs for structured
globals, and preserving fail-closed direct global materialization.

## Watchouts

- Do not make raw symbol spelling the primary identity path when structured
  `LinkNameId` exists.
- Keep final target relocation selection and TLS lowering out of scope.
- Watch the schema limitation: prepared global addresses currently have only
  `symbol_name`, so the implementation should use named helper boundaries or an
  explicit marker if downstream code must distinguish raw/no-id compatibility.
- Existing direct global materialization already fails closed on missing
  `pointer_symbol_link_name_id`; Step 3 should avoid weakening that path.
- Adjacent control-flow/call-plan raw fallbacks exist but are outside the
  primary prepared-address/materialization construction target unless Step 3
  intentionally broadens ownership.
- The retained compatibility class should not match globals that have
  structured `Global::link_name_id`, even if their raw name text matches.

## Proof

Passed. Ran:
`git diff --quiet -- src/backend/bir src/backend/prealloc tests && printf 'analysis-only proof: no implementation or test diff for raw global missing-ID contract decision\n' > test_after.log`

Proof log: `test_after.log`.
