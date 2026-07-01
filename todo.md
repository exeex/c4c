Status: Active
Source Idea Path: ideas/open/510_rv64_selected_object_data_emission.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Trace RV64 Object-Data Consumption

# Current Packet

## Just Finished

Step 2 of `plan.md` traced prepared selected object-data publication through
contract verification into the RV64 object-data consumer, without code or test
edits.

- Trace artifacts were recorded under
  `build/agent_state/510_step2_object_data_trace/`:
  `trace_summary.md` and `source_locations.txt`.
- Producer fact path: `src/backend/prealloc/prealloc.cpp` calls
  `prepare::populate_prepared_object_data_plans`; publication lives in
  `src/backend/prealloc/object_data.cpp`, which writes
  `PreparedGlobalObjectData` records into `prepared.object_data.globals`.
- Supported publication currently requires an explicit global link label,
  nonzero alignment, nonzero object extent, non-TLS storage, non-GOT-required
  address materialization, and immediate or immediate-element initializer
  bytes. It publishes section kind, object offset/size, alignment, emitted
  bytes or zero-fill count, and the matching contract booleans.
- Unsupported publication uses `unsupported_global_object_data`: it may carry
  label, extent, alignment, publication identity, and unsupported-marker facts,
  but it does not publish consumable emitted bytes, zero-fill payload,
  relocation payload, or intentional section authority for RV64 emission.
- Verifier path:
  `PreparedSelectedObjectDataContractFacts` is classified by
  `classify_prepared_selected_object_data_contract` and reported by
  `verify_prepared_selected_object_data_contract`. Missing facts are
  `ProducerMissing`, conflicting facts are `ProducerIncoherent`,
  `UnsupportedButCoherent` is `TargetUnsupportedButCoherent`, and invalid
  pre-prepared initializer semantics are `PrePreparedSemanticFailure`.
- RV64 consumer entry point:
  `src/backend/mir/riscv/codegen/object_emission.cpp` function
  `append_rv64_prepared_data_objects`. It finds the selected record with
  `prepare::find_prepared_global_object_data`, builds facts with
  `rv64_selected_object_data_contract_facts`, verifies the contract, and emits
  only coherent records plus two existing narrow `unsupported_but_coherent`
  admissions.
- Existing partial support to preserve: coherent initialized globals,
  coherent zero-fill globals, coherent constant globals, selected
  symbol-pointer relocation globals, selected zero-pointer/no-initializer BSS,
  and the current fail-closed diagnostics for missing or incoherent prepared
  data facts.
- Reusable helper boundaries: section selection/creation through
  `object::get_or_create_section`, alignment through `object::align_section`,
  initialized bytes through `object::append_section_bytes`, zero-fill
  reservation through `object::reserve_section_bytes`, symbol definition
  through `object::define_symbol`, duplicate/undefined checks through
  `object::find_symbol` and `object::is_undefined_symbol`, relocation target
  declaration through `rv64_find_or_declare_relocation_symbol`, and relocation
  attachment through `object::attach_relocation`.
- Unsupported variants that must continue to fail closed: missing labels,
  missing publication identity, missing extents/object byte ranges, missing
  emitted-byte authority, missing zero-fill authority, missing or ambiguous
  relocation authority, unsupported marker-only records, thread-local storage,
  GOT-required globals, producer-incoherent records, invalid pre-prepared
  initializer semantics, and records without explicit section/data authority.

## Suggested Next

Execute Step 3: add a small RV64 admission helper for selected object-data
records that are already prepared-authoritative, and keep every other shape on
the verifier diagnostic path. The boundary should require explicit prepared
object label, object extent, alignment, section/data authority, and the exact
payload authority needed by the chosen emission path: emitted bytes for data or
rodata, zero-fill count for BSS, and relocation-ready identity before any
relocation emission.

## Watchouts

- Do not generalize from `src/20000412-1.c`, object label `2`, object size
  `1656`, raw global spelling, C source shape, or testcase identity.
- Do not treat `PreparedGlobalObjectData::section_kind`'s default `Data` value
  as section authority for an `unsupported_but_coherent` marker-only record.
- Current relocation support for selected symbol-pointer globals still reads
  raw BIR initializer shape; Step 3/4 should not broaden relocation emission
  until prepared relocation facts exist.
- Thread-local storage, GOT-required globals, missing static-local object
  labels, parameter homes, unrelated global access widths, and F128 stay out
  of this RV64 consumer slice.
- Keep `unsupported_but_coherent` marker-only records fail-closed unless the
  prepared producer publishes explicit bytes, zero-fill, section, or relocation
  authority for the selected object-data record.

## Proof

- `scripts/plan_review_state.py set-step --step-id 2 --step-title 'Trace RV64 Object-Data Consumption'`
  passed.
- Source trace commands used `rg` and `sed` over
  `src/backend/prealloc/object_data.*`,
  `src/backend/prealloc/prepared_contract_verifier.*`,
  `src/backend/mir/riscv/codegen/object_emission.cpp`,
  `src/backend/mir/object/model.*`, and
  `tests/backend/mir/backend_riscv_object_emission_test.cpp`.
- `git diff --check -- todo.md` is the delegated final proof for this
  trace-only packet.
- `test_after.log` was not produced because this packet's delegated proof is
  the step-state command plus `git diff --check -- todo.md`, not a build/test
  subset.
