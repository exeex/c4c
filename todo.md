Status: Active
Source Idea Path: ideas/open/111_store_source_publication_dump_visibility.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Map Existing Store-Source Facts And Dump Conventions

# Current Packet

## Just Finished

Step 1 mapped the existing store-source publication facts and prepared dump
conventions without implementation edits.

Chosen dump surface: `prepare::print(const PreparedBirModule&)` in
`src/backend/prealloc/prepared_printer.cpp`, via a new bounded prepared-printer
append section near the existing publication-adjacent calls:
`append_call_plans(...)`, `append_select_chain_materializations(...)`, then the
store-source publication visibility section. The section should use the current
prepared-printer style: a stable header like
`--- prepared-store-source-publications ---`, two-space indented rows, and
`key=value` fields with `yes`/`no` booleans.

Existing facts to print from `PreparedStoreSourcePublicationPlan`:
`status`, `intent`, `source_value_name`, `source_producer_kind`,
`source_producer_block_label`, `source_producer_instruction_index`,
`source_load_local`, `source_load_global`, `source_cast`, `source_binary`,
`source_select`, `direct_global_select_chain_source`,
`direct_global_select_chain_root_is_select`, and
`direct_global_select_chain_root_instruction_index`. Keep the visible contract
bounded to source-producer identity and direct-global select-chain facts; do
not dump raw producer pointers or unrelated destination/publication internals.

Existing producer/direct-global sources: `plan_prepared_store_source_publication`
copies source-producer facts from `PreparedStoreSourcePublicationInputs::source_producer`
when the producer kind is not `Unknown`; AArch64 `plan_store_local_source_publication`
already gets that producer from `prepared_store_source_producer(...)`, computes
direct-global facts through
`find_prepared_store_source_direct_global_select_chain_dependency(...)`, and
passes `contains_direct_global_load`, `root_is_select`, and
`root_instruction_index` into the plan. The store-source direct-global helper is
currently an alias of the existing direct-global select-chain query, so the
printer should consume those plan fields rather than re-querying or deriving
them ad hoc.

Prepared-printer conventions to copy: call-plan rows print
`direct_global_select_chain=yes direct_global_source=<value>
direct_global_root_is_select=<yes/no> direct_global_root_inst=<index>`;
select-chain materialization rows print
`select_chain function=<name> block=<label> value=<value> root_is_select=<yes/no>
root_inst=<index> direct_global_select_chain=yes
direct_global_root_is_select=<yes/no> direct_global_root_inst=<index>
source_producer=<kind> source_producer_block=<label>
source_producer_inst=<index>`. Store-source rows should mirror those labels,
using `store_source function=... block=... inst=... source=...`.

Focused fixture/test target: add the visibility assertions in
`tests/backend/bir/backend_prepared_printer_test.cpp` under the existing
`backend_prepared_printer` CTest target. Reuse the local fixture style from
`prepare_select_chain_direct_global_dump_module()` for direct-global select
chains, and adapt the focused store-source planning fixtures from
`tests/backend/mir/backend_store_source_publication_plan_test.cpp`
(`records_cast_source_producer_from_prepared_lookup`,
`finds_store_source_direct_global_select_chain_dependency`, and
`rejects_incomplete_inputs`) into prepared-printer module fixtures.

Access gap: `PreparedStoreSourcePublicationPlan` is public and has the required
facts, but `PreparedBirModule` does not currently store a collection of
store-source plans. The next implementation packet needs a narrow structured
prepared-printer/accessor path that obtains store-source publication plans from
existing prepared module facts without adding printer-local source-producer or
select-chain authority.

## Suggested Next

Execute Step 2 from `plan.md`: add a bounded
`prepared-store-source-publications` dump section and any minimal structured
access needed so the printer formats existing `PreparedStoreSourcePublicationPlan`
facts only.

## Watchouts

- Do not add printer-side source-producer or select-chain derivation; if a
  helper must construct store-source plans for dumping, keep the authority in
  existing publication/lookup helpers.
- `PreparedStoreSourcePublicationPlan` has producer instruction pointers; the
  dump should print stable kind/block/index facts, not raw pointer values.
- The likely row labels are `source_producer`, `source_producer_block`,
  `source_producer_inst`, `direct_global_select_chain`,
  `direct_global_root_is_select`, and `direct_global_root_inst`.
- Tests must assert concrete source-producer and direct-global select-chain
  fields, not just a section header.
- Preserve missing-source and missing-destination-access fail-closed behavior.

## Proof

Ran `bash -lc 'git diff --check > test_after.log 2>&1'`; passed. Proof log:
`test_after.log`.
