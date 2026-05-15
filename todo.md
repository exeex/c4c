Status: Active
Source Idea Path: ideas/open/233_aarch64_global_address_materialization.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Add Structured Address-Kind Carriers

# Current Packet

## Just Finished

Step 2 added the first prepared-stage structured address-materialization
carrier beside `PreparedMemoryAccess`.

Completed work:

- Added `PreparedAddressMaterializationKind` and
  `PreparedAddressMaterialization` under prepared addressing, with structured
  kind, result value name/id/home fields, symbol/text identity, byte offset,
  address space, and TLS facts.
- Populated direct global/TLS-global carriers from
  `bir::Value::pointer_symbol_link_name_id`, and populated string-constant
  carriers from pointer-typed instruction results with
  `MemoryAddress::BaseKind::StringConstant`.
- Kept the new records separate from load/store `PreparedMemoryAccess`; no
  selected AArch64 lowering or AArch64 printer path was added.
- Added explicit prepared notes when an address-looking pointer result lacks
  required structured symbol/text identity.
- Extended prepared addressing debug output and focused prepared stack-layout
  tests to prove direct global, TLS-global, and string-constant carriers.

## Suggested Next

Step 3 first implementation packet target: add selected AArch64 address
materialization records for the prepared `DirectGlobal`/`TlsGlobal` and
`StringConstant` carrier facts, but keep terminal printing deferred until the
selected machine nodes carry every relocation operand the printer needs.

## Watchouts

Population currently records result value names at stack-layout time; later
value-home/id fields are available on the carrier but not backfilled by this
packet. Label and GOT kinds are represented in the enum for the planned route,
but no label/GOT population or selected lowering has landed yet. The population
hook lives in `src/backend/prealloc/stack_layout/coordinator.cpp`, because that
is where prepared addressing is actually built.

## Proof

Ran delegated proof:

`(cmake --build build -j2 && ctest --test-dir build -j --output-on-failure -R '^backend_') > test_after.log 2>&1`

Result: passed; 139 backend tests passed. Proof log: `test_after.log`.
