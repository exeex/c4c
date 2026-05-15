Status: Active
Source Idea Path: ideas/open/233_aarch64_global_address_materialization.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Populate Label Address Materialization

# Current Packet

## Just Finished

Step 5 populated label address materialization through prepared addressing and
AArch64 selected machine records without adding terminal label printer output.

Completed work:

- Added `target_label` to prepared address-materialization carriers and the
  prepared printer dump, preserving structured `BlockLabelId` identity for
  label address producers.
- Recognized BIR label-backed memory addresses when pointer-valued load results
  materialize a label address, including byte offset and address-space facts,
  and diagnosed missing target labels instead of inferring from rendered text.
- Added `LabelPageLow12` as a distinct AArch64 address-materialization kind so
  label records can select without being mistaken for direct globals.
- Selected prepared `Label` carriers into `AddressMaterializationRecord` values
  with result register authority, target label identity/name, byte offset, and
  branch-target operand/effect facts.
- Added focused prepared and AArch64 MIR tests for label carrier population,
  selected label records, and missing-label diagnostics.

## Suggested Next

Next packet should execute Step 6 from `plan.md`: populate/select GOT
address-materialization facts as structured records while keeping terminal GOT
printer output deferred until its relocation sequence is fully specified.

## Watchouts

Label records now select as `LabelPageLow12`, but terminal label printer output
is still intentionally absent and remains Step 8 work. The selected kind is
separate from `DirectPageLow12` to avoid accidentally routing labels through
the direct-global printer path. GOT and TLS policy must remain explicit and
must not be inferred from rendered names.

## Proof

Ran delegated proof:

`(cmake --build build -j2 && ctest --test-dir build -j --output-on-failure -R '^backend_') > test_after.log 2>&1`

Result: passed; 139 backend tests passed. Proof log: `test_after.log`.
