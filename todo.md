Status: Active
Source Idea Path: ideas/open/510_rv64_selected_object_data_emission.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Rebuild Selected Object-Data Evidence

# Current Packet

## Just Finished

Step 1 of `plan.md` rebuilt the selected object-data evidence for
`tests/c/external/gcc_torture/src/20000412-1.c` without code or test edits.

- 424 handoff artifacts under
  `build/agent_state/424_step2_infrastructure_classification/20000412-1/`
  classify the row as an RV64 object/global-data emission gap:
  `unsupported_global_data`, selected object-data
  `status=unsupported_but_coherent`.
- Fresh `HEAD` probes were captured under
  `build/agent_state/510_step1_object_data_evidence/`:
  `dump-bir.txt`, `dump-prepared-bir.txt`, `codegen-obj.txt.stderr`, and
  matching `.rc` files.
- Prepared addressing is coherent for the access to `i`:
  `dump-prepared-bir.txt` shows `access block=entry inst_index=0
  base=global_symbol result=%t0 symbol=i offset=0 size=2 align=2
  base_plus_offset=yes layout_authority=scalar_layout
  range_verdict=proven_in_bounds`.
- Prepared selected object-data facts from the verifier diagnostic are:
  object label `object_label_id=2`, object extent
  `object_size_bytes=1656`, emitted-byte count `0`, zero-fill count `0`,
  and status `unsupported_but_coherent`.
- Relocation state: no relocation-ready fact is surfaced by the current dump or
  diagnostic for this row; the diagnostic reports no emitted bytes or zero-fill
  payload and names the unsupported object-data marker shape.
- Section authority: the current evidence does not print a consumable prepared
  section authority for this row. Do not infer `.data`, `.rodata`, or `.bss`
  from the C source, the global spelling, object label `2`, or size `1656`.
- Current diagnostic, reproduced by the fresh object-route probe, is
  `error: --codegen obj failed: RISC-V backend object route unsupported
  prepared module shape: unsupported_global_data: prepared selected
  object-data contract status=unsupported_but_coherent object_label_id=2
  object_size_bytes=1656 emitted_byte_count=0 zero_fill_byte_count=0`.
- Current rejection point is
  `src/backend/mir/riscv/codegen/object_emission.cpp` in
  `append_rv64_prepared_data_objects`: after
  `verify_prepared_selected_object_data_contract` classifies the row as
  non-`Coherent`, the gate admits only the existing partial
  `unsupported_but_coherent` zero-fill and symbol-pointer helper shapes; this
  representative matches neither helper, so line 11937 returns the prepared
  object-data diagnostic.
- Current `HEAD` already has partial selected object-data support that must be
  preserved: coherent prepared emitted-byte globals, coherent zero-fill
  globals, coherent symbol-pointer relocation globals, plus narrow
  `unsupported_but_coherent` admissions for zero-pointer BSS and selected
  symbol-pointer data. The remaining unsupported shape here is a coherent
  unsupported-marker object record with label and extent but no emitted bytes,
  zero-fill payload, relocation payload, or explicit section evidence surfaced
  for RV64 emission.

## Suggested Next

Execute Step 2: trace the prepared object-data publication and RV64 consumer
path for selected object records, then define the minimal admission boundary
for records that have explicit prepared section, bytes, zero-fill, and
relocation authority.

## Watchouts

- Keep the route limited to RV64 object emission consuming explicit prepared
  selected object-data facts.
- Do not infer object labels, extents, initialized bytes, zero-fill,
  relocations, section placement, or symbol identity from testcase names,
  source shape, raw globals, or magic constants.
- Do not treat the default `PreparedGlobalObjectData::section_kind` value as
  sufficient section authority for an `unsupported_but_coherent` marker record
  unless a later trace proves that prepared publication intentionally owns that
  field for the selected row.
- Preserve the existing partial support for coherent initialized data,
  coherent BSS, selected symbol-pointer relocations, and the two narrow
  `unsupported_but_coherent` admissions while adding any broader selected
  object-data handling.
- Keep static-local object-label publication, parameter-home publication,
  unrelated global access widths, GOT-required globals, thread-local storage,
  F128, and broad ABI work out of this plan.

## Proof

- `scripts/plan_review_state.py set-step --step-id 1 --step-title 'Rebuild Selected Object-Data Evidence'`
- `./build/c4cll --dump-bir --target riscv64-linux-gnu tests/c/external/gcc_torture/src/20000412-1.c`
  captured as `build/agent_state/510_step1_object_data_evidence/dump-bir.txt`
  with return code `0`.
- `./build/c4cll --dump-prepared-bir --target riscv64-linux-gnu tests/c/external/gcc_torture/src/20000412-1.c`
  captured as
  `build/agent_state/510_step1_object_data_evidence/dump-prepared-bir.txt`
  with return code `0`.
- `./build/c4cll --codegen obj --target riscv64-linux-gnu tests/c/external/gcc_torture/src/20000412-1.c -o build/agent_state/510_step1_object_data_evidence/20000412-1.o`
  captured as
  `build/agent_state/510_step1_object_data_evidence/codegen-obj.txt`,
  `.stderr`, and `.rc`; return code is `2`, matching the expected current
  RV64 object-route rejection.
- `git diff --check -- todo.md` passed.
- `test_after.log` was not produced because the delegated proof requested the
  focused probe artifacts above as the non-regression evidence for this
  evidence-only packet.
