Status: Active
Source Idea Path: ideas/open/233_aarch64_global_address_materialization.md
Source Plan Path: plan.md
Current Step ID: 8
Current Step Title: Print Remaining Relocation-Aware AArch64 Sequences

# Current Packet

## Just Finished

Step 8 printed the remaining terminal AArch64 address-materialization
sequences from structured `AddressMaterializationRecord` fields.

- `LabelPageLow12` now prints the same page plus low-12 sequence as direct
  globals, using the record's structured target-label text and result register.
- `GotPageLow12` now prints a GOT load sequence from explicit symbol and
  `GotRequired` policy facts:
  `adrp <result>, :got:<symbol>` then
  `ldr <result>, [<result>, :got_lo12:<symbol>]`.
- `TlsRelative` now prints the local-exec thread-pointer-relative sequence from
  TLS model/thread-pointer/relocation facts:
  `mrs <result>, tpidr_el0`,
  `add <result>, <result>, :tprel_hi12:<symbol>`, and
  `add <result>, <result>, :tprel_lo12_nc:<symbol>`.
- Nonzero prepared byte offsets still append an immediate `add` after GOT or
  TLS base materialization; direct, string, and label keep the existing
  relocation-offset spelling.
- Printer tests cover label, GOT, and TLS output plus fail-closed missing GOT
  policy and missing label text diagnostics.

## Suggested Next

Plan-owner review/close decision for idea 233, or the next packet should be
only residual validation/cleanup if the plan has an explicit remaining step.

## Watchouts

- Output is driven by selected record fields and result registers, not by the
  archived implicit `x0` scratch convention.
- GOT printing requires the selected record's explicit `GotRequired` policy;
  missing policy remains fail-closed.
- Label printing requires structured `target_label_name`; missing label text
  remains fail-closed.
- TLS output is currently the local-exec AArch64 form represented by the
  selected TLS facts.

## Proof

`(cmake --build build -j2 && ctest --test-dir build -j --output-on-failure -R '^backend_') > test_after.log 2>&1`

Result: passed, 139/139 backend tests green. Proof log: `test_after.log`.
