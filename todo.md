Status: Active
Source Idea Path: ideas/open/688_initializer_lowering_bridge_isolation.md
Source Plan Path: plan.md
Current Step ID: Step 4
Current Step Title: Narrow Initializer Value Materialization

# Current Packet

## Just Finished

Step 4 completed the initializer value materialization narrowing in
`src/backend/bir/lir_to_bir/global_initializers.cpp`.

Exact files/helpers touched:
- Added `AggregateInitializerMaterialization` as the adapter-local carrier for
  type declarations, structured-layout compatibility, emitted values, pointer
  initializer offsets, and pointer value-index publication.
- Added pointer-specific helpers for parsing, recording, and clearing aggregate
  pointer initializer slots so the spelling-to-fact conversion stays private to
  initializer lowering.
- Split aggregate materialization into scalar, array, struct, and zero-fill
  helpers while keeping the exported `lower_aggregate_initializer*()` entry
  points and recursive behavior unchanged.

Behavioral scope: no scalar, byte-string, array, aggregate, pointer initializer
parsing semantics, relocation spelling, prepared object-data publication, BIR
output, diagnostics, runtime behavior, expectations, unsupported markers,
tests, allowlists, or harness policy were changed.

## Suggested Next

Next coherent packet: Step 5 should verify known global-address import
boundaries, focusing on whether known global-address publication and consumers
remain local to the LIR-to-BIR adapter without changing emitted BIR facts or
downstream prepared/target behavior.

## Watchouts

- The public `lower_aggregate_initializer*()` helper signatures remain
  unchanged; this packet narrowed implementation ownership only.
- Pointer initializer value indices still record `out->size()` before pushing
  the named pointer value, preserving relocation-slot/value-index publication
  order.
- The byte-string and integer-array fast paths still run before recursive array
  descent; Step 5 should avoid turning known global-address cleanup into
  semantic initializer repair.

## Proof

`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_|string_authority_guard$)'`

Result: passed. Build completed and 303 selected tests passed with 0 failures,
including `string_authority_guard`. Fresh proof output is preserved in
`test_after.log`.
