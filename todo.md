Status: Active
Source Idea Path: ideas/open/233_aarch64_global_address_materialization.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Print Relocation-Aware AArch64 Sequences

# Current Packet

## Just Finished

Step 4 added terminal AArch64 printer support for selected direct page+low12
address-materialization records while keeping GOT/label/TLS printer paths
explicitly deferred.

Completed work:

- Added printer-facing relocation labels to `AddressMaterializationRecord`,
  populated from prepared name tables during selected record construction.
- Printed selected `DirectPageLow12` direct-global records as structured `adrp`
  plus low-12 `add` sequences using the record's result register, symbol label,
  and byte offset.
- Printed selected `StringConstant` address records through the same page+low12
  sequence using the record's text label and result register.
- Kept TLS materialization explicitly unsupported in the printer with a
  deferred-path diagnostic, and preserved existing non-selected diagnostics for
  GOT/label-style deferred records.
- Added focused AArch64 MIR printer coverage for direct global output, string
  constant output, TLS defer diagnostics, and GOT deferred-selection behavior.

## Suggested Next

Next packet should decide the next lifecycle action for the active idea:
either review/close the current runbook if direct page+low12 materialization is
the intended milestone, or define a follow-up packet for fully specified
GOT/label/TLS relocation sequences before enabling those printer paths.

## Watchouts

The address-materialization opcode still has no generic primary mnemonic; the
printer emits the two-line sequence directly from structured record fields.
TLS records are selected but intentionally fail closed at terminal printing
until their relocation sequence is specified. GOT and label records remain
deferred before printer dispatch and should not gain text templates without
structured relocation policy.

## Proof

Ran delegated proof:

`(cmake --build build -j2 && ctest --test-dir build -j --output-on-failure -R '^backend_') > test_after.log 2>&1`

Result: passed; 139 backend tests passed. Proof log: `test_after.log`.
