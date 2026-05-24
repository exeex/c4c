Status: Active
Source Idea Path: ideas/open/prealloc-publication-accessor-contracts.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Audit The Publication Accessor Surface

# Current Packet

## Just Finished

Completed Step 1 from `plan.md`: audited the publication/accessor target
headers and prepared-printer mirrors without changing implementation or header
files.

Public declaration map:

- Publication plans (`publication_plans.hpp`):
  - `PreparedScalarPublicationStatus` and
    `prepared_scalar_publication_status_name`: scalar publication diagnostics
    for codegen-facing value publication.
  - `PreparedScalarPublicationHookKind` and
    `prepared_scalar_publication_hook_kind_name`: scalar publication hook
    routing over register, stack-slot, rematerializable-immediate, and pointer
    base-plus-offset homes.
  - `PreparedScalarPublicationPlan`,
    `PreparedScalarPublicationInputs`,
    `prepared_scalar_publication_available`,
    `prepared_publication_storage_encoding_from_home`, and
    `plan_prepared_scalar_publication`: scalar publication records and lookup
    facts derived from `PreparedValueHome` plus optional block-entry
    publication context.
  - `PreparedStoreSourcePublicationStatus` and
    `prepared_store_source_publication_status_name`: store-source publication
    diagnostics.
  - `PreparedStoreSourcePublicationIntent` and
    `prepared_store_source_publication_intent_name`: store-source publication
    intent labels for local/global store publication and pointer-store
    writeback.
  - `PreparedStoreSourcePublicationPlan`,
    `PreparedStoreSourcePublicationInputs`,
    `prepared_store_source_publication_available`, and
    `plan_prepared_store_source_publication`: store-source publication records
    combining source value, destination memory access, recovered source value,
    source home, destination stack/frame facts, and pointer-base home facts.
- Prepared lookups (`prepared_lookups.hpp`):
  - `PreparedPriorPreservedValueEntry`: call-plan lookup helper record for
    prior preserved values.
  - `PreparedCallPlanLookups`: stable call-plan indexes by source position,
    prior preserved values, and first stack-preserved values per call.
  - `PreparedAddressMaterializationLookups`: addressing accessor index from
    block labels to materializations.
  - `PreparedMoveBundleLookups`: value-location move-bundle index by phase and
    source position.
  - `PreparedValueHomeLookups`: value-home accessors by prepared value id and
    interned value name.
  - `PreparedFunctionLookups`: aggregate per-function accessor package over
    call plans, address materializations, move bundles, and value homes.
  - `prepared_call_position_key`, `prepared_move_bundle_position_key`, and
    `prepared_prior_preserved_value_entry_position_less`: index key/order
    helpers.
  - `make_prepared_call_plan_lookups`,
    `make_prepared_address_materialization_lookups`,
    `make_prepared_move_bundle_lookups`,
    `make_prepared_value_home_lookups`, and
    `make_prepared_function_lookups`: lookup builders over prepared module
    data.
  - `find_indexed_prepared_call_plan`,
    `find_indexed_prepared_address_materializations`,
    `collect_prepared_address_materializations_for_block`,
    `find_indexed_prepared_move_bundle`,
    `find_latest_indexed_prior_preserved_value`,
    `find_dominating_indexed_prior_preserved_value`, and
    `first_indexed_stack_preserved_values_for_call`: stable accessor helpers
    with indexed fast paths and source-data fallbacks.
- Decoded home storage (`decoded_home_storage.hpp`):
  - `PreparedDecodedHomeStorageSource` and
    `prepared_decoded_home_storage_source_name`: authority source labels for
    none, regalloc assignment, storage plan, and value home.
  - `PreparedDecodedHomeStorageKind` and
    `prepared_decoded_home_storage_kind_name`: decoded operand/storage shape
    labels.
  - `PreparedDecodedHomeStorageStatus` and
    `prepared_decoded_home_storage_status_name`: availability and unsupported
    authority diagnostics.
  - `PreparedDecodedHomeStorage`: decoded home/storage view consumed by later
    operand materialization.
  - `PreparedDecodedHomeStorageDiagnosticCategory`,
    `prepared_decoded_home_storage_diagnostic_category_name`, and
    `PreparedDecodedHomeStorageDiagnostic`: user-facing diagnostic grouping
    for missing or unsupported decoded authority.
  - `PreparedHomeStorageDecodeInputs`,
    `prepared_decoded_home_storage_available`,
    `decode_prepared_regalloc_assignment`,
    `decode_prepared_storage_plan_value`,
    `decode_prepared_value_home`,
    `decode_prepared_home_storage`, and
    `build_prepared_decoded_home_storage_diagnostic`: decoding entry points and
    diagnostic construction.
- Storage plans (`storage_plans.hpp`):
  - `populate_storage_plans`: storage publication population entry point over
    a prepared module. The public storage data records it fills live in
    `storage.hpp` as `PreparedStoragePlanValue`,
    `PreparedStoragePlanFunction`, and `PreparedStoragePlans`.
- Value locations (`value_locations.hpp`):
  - `PreparedValueHomeKind` and `prepared_value_home_kind_name`: value-home
    storage family labels.
  - `PreparedMovePhase` and `prepared_move_phase_name`: prepared move-bundle
    phase labels.
  - `PreparedValueHome`: value-home publication record for register,
    stack-slot, rematerializable-immediate, and pointer base-plus-offset homes.
  - `PreparedMoveBundle`,
    `prepared_move_resolution_has_out_of_ssa_parallel_copy_authority`, and
    `prepared_move_bundle_has_out_of_ssa_parallel_copy_authority`: move-bundle
    records and authority checks.
  - `PreparedValueLocationFunction` and `PreparedValueLocations`: function and
    module containers for value homes and move bundles.
  - `PreparedBlockEntryPublicationStatus`,
    `prepared_block_entry_publication_status_name`,
    `PreparedBlockEntryPublication`,
    `prepared_block_entry_publication_available`, and
    `collect_prepared_block_entry_publications`: block-entry publication facts
    derived from out-of-SSA block-entry move bundles.
  - `find_prepared_value_location_function`,
    `find_prepared_value_home`,
    `find_indexed_prepared_value_home`,
    `find_prepared_value_id`,
    `find_indexed_prepared_value_id`,
    `find_prepared_move_bundle`,
    `find_prepared_out_of_ssa_parallel_copy_move_for_step`, and
    `find_prepared_unique_move_bundle`: value-location accessors over
    function, value, phase, and parallel-copy coordinates.
- Prepared-printer mirrors (`prepared_printer/private.hpp`):
  - `append_value_locations`: printer mirror for value homes, move bundles,
    block labels, move destinations, storage kinds, op kinds, ABI bindings,
    and register placements.
  - `append_storage_plans`: printer mirror for storage-plan values, storage
    encodings, banks, register placement, spill slots, stack offsets,
    immediates, and symbols.
  - Other declarations in `private.hpp` are outside this packet's data-family
    targets but share the same broad printer-private aggregate header.

Include dependency observations:

- `publication_plans.hpp` uses declarations from `addressing.hpp` for memory
  access/address base facts, `calls.hpp` for storage encoding kinds,
  `frame.hpp` for frame/stack slot facts, `names.hpp` for interned ids,
  `value_locations.hpp` for value homes and block-entry publications, and
  `../bir/bir.hpp` for `bir::Value` plus F128 payloads. No obvious
  text-only stale include was proven here.
- `prepared_lookups.hpp` uses `addressing.hpp`, `calls.hpp`,
  `control_flow.hpp`, and `value_locations.hpp` directly in public lookup
  records and function declarations. `<optional>` is a Step 2 candidate:
  declarations in this header did not show direct `std::optional` use.
- `decoded_home_storage.hpp` publicly couples decoded storage to
  `PreparedValueHomeLookups`, regalloc values, storage-plan records, and
  value-location records through `prepared_lookups.hpp`, `regalloc.hpp`,
  `storage.hpp`, and `value_locations.hpp`. `prepared_lookups.hpp` may be
  broader than the pointer field needs, but the inline value-home lookup
  template used by the implementation currently lives in
  `value_locations.hpp`, so Step 2 should avoid speculative forward-declare
  churn.
- `storage_plans.hpp` includes `module.hpp` only to expose
  `populate_storage_plans(PreparedBirModule&)`; this is likely broader than a
  forward declaration would require, but changing it should be done with a
  local build because `module.hpp` also aggregates storage/value-location
  contracts.
- `value_locations.hpp` uses `names.hpp`, `regalloc.hpp`, and `../bir/bir.hpp`
  in public declarations and inline helpers. The include is intentionally
  aggregate-heavy because move resolution, ABI binding, register placement,
  names, and BIR F128 payloads all appear in the public surface.
- `prepared_printer/private.hpp` includes `../module.hpp` for every printer
  helper declaration even though declarations need only `PreparedBirModule`
  plus `<iosfwd>`. This is a possible Step 2 contraction candidate, but it is a
  shared printer-private header used by many non-target printer files, so keep
  any change conservative.
- Target `.cpp` include notes: `publication_plans.cpp` only includes its
  header; `prepared_lookups.cpp` adds `module.hpp` and `<algorithm>`;
  `decoded_home_storage.cpp` adds `<algorithm>`; `storage_plans.cpp` adds
  `target_register_profile.hpp`, `<algorithm>`, `<optional>`, `<string>`,
  `<unordered_map>`, and `<vector>`; printer mirrors include `private.hpp` plus
  local formatting/string headers. These match implementation-local helper use.

Stale wording and helper-name candidates:

- `value_locations.hpp:83` comment starts with `Step 5 fence` and says value
  homes and move bundles are `backend-prepared route-local lookup state`. The
  ownership idea is still relevant, but the lifecycle-step marker is stale and
  should be converted to durable package wording.
- `value_locations.hpp:204` comment says `spelling bridge` and `legacy
  callers`. The string-name overload is still live in codegen and prealloc
  helper call sites, so deletion is not proven. Step 2 can rewrite the comment
  to describe a text-name accessor compatibility boundary without claiming
  old lifecycle state.
- `value_locations.hpp:303` comment starts with `Step 5 fence` for local
  parallel-copy step coordinates. The content is useful, but the step marker is
  stale.
- `value_locations.hpp:326` comment starts with `Step 5 fence` and says
  uniqueness is scoped to one prepared route phase/block index. The route-local
  warning is useful, but the step marker is stale.
- No `compat`, `fallback`, `temporary`, `TODO`, `FIXME`, or old-route wording
  was found in the audited target files. The only direct `bridge`/`legacy`
  wording found was the live string-name value-home accessor comment above.

Prepared-printer mirror map:

- `prepared_printer/value_locations.cpp::append_value_locations` mirrors
  `PreparedValueLocations`, `PreparedValueLocationFunction`,
  `PreparedValueHome`, `PreparedValueHomeKind`,
  `PreparedMoveBundle`, `PreparedMovePhase`, move authority, move destination,
  move storage, move op kind, ABI binding, register placement, and
  parallel-copy labels. Dump labels include `--- prepared-value-locations ---`,
  `prepared.func`, `home`, `kind=`, `move_bundle`, `authority=`,
  `source_parallel_copy=`, `move`, `destination_kind=`,
  `destination_storage=`, `op_kind=`, and `abi_binding`.
- `prepared_printer/storage.cpp::append_storage_plans` mirrors
  `PreparedStoragePlans`, `PreparedStoragePlanFunction`,
  `PreparedStoragePlanValue`, `PreparedStorageEncodingKind`,
  register bank/placement, spill-slot placement, register occupancy, stack
  offsets, immediates, and symbol names. Dump labels include
  `--- prepared-storage-plans ---`, `prepared.func`, `storage`,
  `encoding=`, `bank=`, `placement=`, `spill_slot=`, `units=`,
  `slot_id=`, `stack_offset=`, `imm_i32=`, `imm_f128=`, and `symbol=`.
- There is no prepared-printer mirror for `PreparedScalarPublicationPlan`,
  `PreparedStoreSourcePublicationPlan`, `PreparedFunctionLookups`, or
  `PreparedDecodedHomeStorage`; those are codegen/accessor planning and
  diagnostic views, not standalone prepared dump families.

## Suggested Next

Execute Step 2 from `plan.md`: tighten only proven local includes and ownership
comments. Recommended first slice: remove or justify the apparent unused
`<optional>` include in `prepared_lookups.hpp`, replace `Step 5 fence` wording
in `value_locations.hpp` with durable publication/accessor comments, and leave
`storage_plans.hpp` / `prepared_printer/private.hpp` aggregate-include
contraction as deferred unless a local build proves it cleanly.

## Watchouts

- Keep this slice behavior-preserving.
- Do not split `module.hpp`, `regalloc.hpp`, or broad aggregate prealloc
  contracts.
- Do not make printer-only taxonomy changes unless they mirror real data-family
  naming or grouping changes.
- Reject broad codegen include churn, duplicate decode/home-selection logic, or
  semantic changes claimed as layout cleanup.

## Proof

Ran delegated proof `git diff --check`: passed.
