Status: Active
Source Idea Path: ideas/open/233_aarch64_global_address_materialization.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Select AArch64 Address Materialization Nodes

# Current Packet

## Just Finished

Step 3 added selected AArch64 machine-node records that consume the prepared
address-materialization carriers from Step 2, without adding terminal printer
output.

Completed work:

- Added `AddressMaterializationRecord` as a structured AArch64 machine-node
  payload with a non-printing `MachineOpcode::AddressMaterialization`.
- Selected prepared `DirectGlobal`, `TlsGlobal`, and `StringConstant` carriers
  into machine records carrying prepared kind, selected address kind, result
  value id/name/home/register, symbol or text identity, byte offset, address
  space, and TLS facts.
- Wired block dispatch to select address materialization from prepared
  addressing carriers before scalar fallback, and to record the emitted result
  register for later same-block users such as returns.
- Left `Label` and `GotGlobal` carriers as explicit deferred-unsupported
  address materialization records; missing symbol/text/result/register facts
  fail closed with typed record errors.
- Added focused AArch64 MIR coverage in
  `backend_aarch64_prepared_memory_operand_records_test.cpp` proving direct
  global, TLS, string-constant, unsupported-kind, missing-identity, and dispatch
  selection behavior.

## Suggested Next

Step 4 first implementation packet target: add terminal printer support for
the selected address-materialization records, starting with direct page+low12
and preserving the existing deferred diagnostics for GOT/label/TLS paths until
their relocation sequences are fully specified.

## Watchouts

The selected address-materialization opcode intentionally has no printer
mnemonic yet, so terminal printer paths remain untouched. The selected node
uses the prepared value-location/storage-plan register as the result authority;
non-register result homes are rejected for now. TLS carriers are selected into
structured records with TLS facts, but terminal TLS emission still needs a
policy-specific printer/lowering sequence.

## Proof

Ran delegated proof:

`(cmake --build build -j2 && ctest --test-dir build -j --output-on-failure -R '^backend_') > test_after.log 2>&1`

Result: passed; 139 backend tests passed. Proof log: `test_after.log`.
