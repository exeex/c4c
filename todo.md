Status: Active
Source Idea Path: ideas/open/247_explicit_got_materialization_policy.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Consume GOT Policy In AArch64 Selection

# Current Packet

## Just Finished

Step 3 consumed prepared `GotGlobal` address-materialization carriers in
AArch64 selected records without adding terminal GOT printer output.

- Added `AddressMaterializationKind::GotPageLow12` as the selected AArch64
  record kind for prepared `GotGlobal` carriers.
- Added `address_materialization_policy` to `AddressMaterializationRecord` so
  selected records preserve the explicit prepared `GotRequired` policy.
- Updated prepared address-materialization record construction so `GotGlobal`
  requires explicit `GotRequired` policy, structured symbol identity, non-TLS
  facts, result value home, and register storage before it selects.
- Kept direct globals selecting as `DirectPageLow12` and preserved existing
  direct printer fixtures by requiring direct policy on the prepared conversion
  path, not on hand-built printer records.
- Extended focused AArch64 MIR tests to prove GOT selected records, dispatch
  consumption from prepared carriers, and fail-closed missing/mismatched GOT
  policy diagnostics.
- Fixed the Step 3 terminal-printer acceptance issue by making selected
  `GotPageLow12` and `LabelPageLow12` records explicitly fail closed with
  deferred-printer diagnostics instead of falling through to empty relocation
  labels.

## Suggested Next

Step 4 implementation packet: define terminal AArch64 GOT printer behavior only
after the GOT relocation operand sequence is fully specified.

## Watchouts

- Terminal GOT and label printer output is still deferred. The printer now
  rejects selected `GotPageLow12` and `LabelPageLow12` records explicitly until
  their relocation sequences are specified.
- `GotPageLow12` records now have selected machine-node status, so future
  printer work must handle that kind explicitly before routing GOT records to
  terminal output.
- Do not infer GOT from rendered names or `is_extern`; the AArch64 selector now
  depends on the prepared `GotGlobal` kind plus explicit `GotRequired` policy.
- TLS/GOT remains unsupported and fails closed through the prepared record
  validation path.

## Proof

`(cmake --build build -j2 && ctest --test-dir build -j --output-on-failure -R '^backend_') > test_after.log 2>&1`

Result: passed, 139/139 backend tests green. Proof log: `test_after.log`.
