Status: Active
Source Idea Path: ideas/open/483_bir_semantic_local_memory_scalar_load_producer.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Produce Scalar Local Loads From Provenance

# Current Packet

## Just Finished

Implemented Step 2: production semantic scalar local-memory load facts now
publish through `local_array_scalar_local_loads` only from matching available
`local_array_local_address_provenances`. The record packages the consuming
`LoadLocalInst`, loaded value identity, source object, element address identity,
dynamic index, scalar element type/size, block/instruction coordinate, and the
provenance status. Fail-closed behavior is covered for missing provenance,
non-available provenance, aggregate/member, integer-pointer round-trip, global
source object, variadic/runtime boundary, unsupported type, bootstrap boundary,
raw-shape-only evidence, target-only evidence, and coordinate confusion.
Follow-up repair in this packet tightened availability so shifted loads with
nonzero `LoadLocalInst::byte_offset` or nonzero address byte offset fail closed
as coordinate confusion instead of becoming available from base-pointer identity
alone.

## Suggested Next

Execute Step 3 from `plan.md`: record whether downstream scalar-load consumers
or lowering work can resume from `local_array_scalar_local_loads`.

## Watchouts

- Consumers should use `local_array_scalar_local_loads`; they should not
  re-derive local object, element offset, layout, range, or provenance from
  provenance records, checker inputs, lower proof surfaces, final homes, raw
  testcase shape, names, or target behavior.
- Available scalar load facts require exact element-address consumption:
  nonzero load or address byte offsets remain unavailable.
- RV64/MIR lowering, object emission, store/call/memcpy/memset producers,
  aggregate/member producers, variadic/byval/va_arg work, volatile/atomic work,
  complex/vector/F128 work, and broad generic load analysis were not
  implemented in this packet.

## Proof

Delegated Step 2 proof:

```sh
cmake --build build -j2 > test_after.log 2>&1 && ctest --test-dir build -j2 --output-on-failure -R '^backend_' >> test_after.log 2>&1 && git diff --check >> test_after.log 2>&1
```

Result: passed. Output preserved in `test_after.log`.
