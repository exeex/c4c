Status: Active
Source Idea Path: ideas/open/value-home-storage-interpretation-prealloc.md
Source Plan Path: plan.md
Current Step ID: Step 4
Current Step Title: Prove Reuse Path For x86 Prepared Operands

# Current Packet

## Just Finished

Completed `Step 4: Prove Reuse Path For x86 Prepared Operands`.

Added a narrow x86 prepared reuse path in
`src/backend/mir/x86/prepared/prepared.hpp`:

- `prepared::Query::regalloc()` exposes the prepared regalloc function for the
  query focus;
- `prepared::Query::storage_plan()` exposes the prepared storage plan for the
  query focus;
- `prepared::Query::decode_home_storage()` calls the shared prealloc
  `prepare::decode_prepared_home_storage()` helper and returns
  `prepare::PreparedDecodedHomeStorage`.

This gives x86 prepared operand/lowering code a concrete API to reuse decoded
Prepared home/storage interpretation without re-decoding raw regalloc,
storage-plan, or value-home records. x86-specific operand spelling and
encoding remain outside prealloc.

Added `backend_x86_prepared_decoded_home_storage` coverage proving:

- x86 prepared `Query` exposes regalloc, storage-plan, and value-location
  inputs;
- `Query::decode_home_storage()` preserves regalloc over storage-plan
  precedence;
- storage-plan immediates are returned as decoded facts;
- storage-plan `None` authority blocks value-home fallback;
- true no-record fallback reaches value homes;
- a missing query focus returns no authority.

## Suggested Next

Execute the next coherent packet by closing or reviewing the active plan,
depending on whether the source idea requires another integration step beyond
the AArch64 adapter and x86 prepared reuse proof.

## Watchouts

- `Query::decode_home_storage()` returns decoded facts only; it does not render
  x86 operand text or encode target-specific addressing.
- The helper currently builds decode inputs from query focus state and uses
  linear value-home lookup by passing no prebuilt value-home index. That keeps
  this proof narrow; a later performance-oriented x86 lowering integration may
  pass precomputed lookups if needed.
- The new x86 test is a direct Prepared fixture, not a lowering rewrite.

## Proof

Ran exactly:

`{ cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_'; } > test_after.log 2>&1`

Result: passed. `test_after.log` reports `100% tests passed, 0 tests failed out
of 152`; `backend_x86_prepared_decoded_home_storage` passed.
