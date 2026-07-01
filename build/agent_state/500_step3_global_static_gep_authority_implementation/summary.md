# 500 Step 3 Global/Static GEP Authority Implementation

## Packet

Implemented production `global_static_gep_authority` certificate publication
during BIR global GEP lowering. Final semantic global/static GEP admission,
RV64/MIR lowering, and object emission consumers remain intentionally
unimplemented.

## Record Surface

Added `bir::GlobalStaticGepAuthorityRecord` with explicit fields for:

- availability status and derivation kind
- global name and `LinkNameId`
- result pointer name and base pointer name
- source/layout path type text
- source extent, element byte offset, element type, element size, count, and stride
- constant-range or dynamic-range evidence
- dynamic index identity
- layout authority and range verdict
- LIR producer function, block label, instruction index, and lookup key

The default-constructed record now fails closed as
`MissingGlobalSourceObject`; production publication must explicitly set
`Available` after all required authority checks pass.

The status vocabulary distinguishes available certificates from missing global
identity, missing layout/path/range facts, out-of-bounds ranges, pointer/string
global provenance boundaries, runtime/string intrinsic boundaries,
aggregate/member boundaries, raw-shape-only, target-only, and coordinate
confusion.

## Producer Publication

`BirFunctionLowerer::lower_memory_gep_inst` now publishes records from the same
successful global GEP lowering branches that already capture production
authority:

- direct global constant GEP
- relative global-pointer constant GEP
- dynamic global pointer-array GEP
- dynamic global aggregate-array GEP
- dynamic global scalar-array GEP

Available records require global/static identity, known source extent, layout
path text, element byte range, constant or dynamic range facts, in-bounds range
verdict, and available LIR producer coordinate. Pointer-valued global GEPs are
published as `StringOrGlobalPointerProvenanceBoundary`, not `Available`, so
`src/20051104-1.c` remains prerequisite-owned. Runtime/string-intrinsic
consumers such as `src/ieee/copysign2.c` remain fail-closed for the later final
admission consumer packet.

## Coverage

Focused backend coverage added to `backend_lir_to_bir_notes`:

- constant `[4 x i32]` global element GEP with LinkNameId publishes exactly one
  `Available` direct-global authority record with byte offset 8, element size
  4, proven in-bounds constant range, and the `entry`/instruction-0 LIR
  producer coordinate
- default-constructed `GlobalStaticGepAuthorityRecord` is not `Available`
- raw/no-id dynamic global GEP lowering still publishes records, but every
  record fails closed; the test requires a `MissingGlobalIdentity` record for
  `cases` and rejects any `Available` record

## Proof

Focused pre-proof probes passed:

```sh
cmake --build build -j2
ctest --test-dir build -j2 --output-on-failure -R '^backend_lir_to_bir_notes$'
```

Delegated proof command:

```sh
cmake --build build -j2 > test_after.log 2>&1 && ctest --test-dir build -j2 --output-on-failure -R '^backend_' >> test_after.log 2>&1 && git diff --check >> test_after.log 2>&1
```

Delegated proof result: passed after fail-closed default repair. Output
preserved in `test_after.log`.
