# 500 Step 4 Semantic Global/Static GEP Admission

## Packet

Implemented final semantic global/static GEP admission publication from
matching available `global_static_gep_authority` records. This packet does not
implement RV64/MIR lowering, object emission, or downstream consumers.

## Record Surface

Added `bir::GlobalStaticSemanticGepRecord` and
`bir::GlobalStaticSemanticGepInputs`. The semantic admission record carries a
pointer to the consumed `GlobalStaticGepAuthorityRecord` and copies the
authority identity, derivation kind, global/link identity, source/layout path,
element byte range, range facts, range verdict, and LIR producer coordinate
fields.

The default record is fail-closed as `MissingGlobalSourceObject`. The evaluator
preserves non-available authority statuses, including pointer/string,
runtime/string intrinsic, raw-shape-only, target-only, and coordinate-confusion
boundaries. It publishes `Available` only when the authority itself is
`Available` and still carries complete global identity, source extent, layout
path, element byte range, constant or dynamic range facts, proven in-bounds
verdict, non-pointer element type, and an available LIR producer coordinate.

## Publication

Added `prepare::populate_global_static_semantic_geps`, which clears and
repopulates `function.global_static_semantic_geps` from
`function.global_static_gep_authorities`. Duplicate authority identities
matching on producer lookup key, global name, result name, and byte offset are
published as `PreparedBirCoordinateConfusion`.

The existing `populate_local_array_scalar_local_loads` flow invokes the new
global/static semantic GEP population pass, so normal prealloc publication
reaches this final semantic producer surface without requiring target lowering
changes.

## Coverage

Focused coverage in `backend_publication_plan_record` verifies:

- missing authority fails closed as `MissingGlobalSourceObject`
- available authority publishes one available semantic global/static GEP
  record and preserves pointer identity to the consumed authority
- string/global-pointer boundary authority remains fail-closed
- coordinate-confused authority remains fail-closed
- duplicate authority identities publish coordinate-confusion records

## Proof

Focused pre-proof probes passed:

```sh
cmake --build build -j2
ctest --test-dir build -j2 --output-on-failure -R '^backend_publication_plan_record$'
```

Delegated proof command:

```sh
cmake --build build -j2 > test_after.log 2>&1 && ctest --test-dir build -j2 --output-on-failure -R '^backend_' >> test_after.log 2>&1 && git diff --check >> test_after.log 2>&1
```

Delegated proof result: passed. Output preserved in `test_after.log`.
