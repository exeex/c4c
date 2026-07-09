Status: Active
Source Idea Path: ideas/open/631_direct_global_symbol_local_memory_policy.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Publish Or Verify Prepared Direct-Global Facts

# Current Packet

## Just Finished

Step 3 added focused prepared-layer coverage in
`tests/backend/bir/backend_prepare_stack_layout_test.cpp` for explicit scalar
`LoadLocalInst` and `StoreLocalInst` direct `GlobalSymbol` local-memory
carriers. The fixture now proves an 8-byte `g.scalar.i64` carrier preserves
result/stored-value metadata, default address space, non-volatile access state,
`PreparedAddressBaseKind::GlobalSymbol`, link-name symbol identity, direct
materialization policy, offset/size/alignment `0/8/8`,
`can_use_base_plus_offset=true`, global provenance identity, complete known
extent, complete requested range, `ScalarLayout`, `ProvenInBounds`, and
`prepared_global_symbol_memory_has_publication_authority(...)` for both load
and store carriers.

Focused fail-closed prepared authority coverage now includes missing symbol
identity, missing base-plus-offset, incomplete extent, missing requested range,
and wrong layout authority using the same local scalar carrier. Existing
prepared-layer coverage still covers raw/no-id structured global spelling
rejection, out-of-range and missing-extent scalar globals, aggregate lane
authority rejection, and TLS/extern/missing-extent array rejection. Volatile,
non-default address space, and unsupported addressing-policy facts are
represented as prepared access/materialization fields rather than part of the
publication-authority predicate, so Step 4 must gate those fields explicitly at
the RV64 consumer boundary.

## Suggested Next

Step 4 consumer packet: add narrow RV64 admission and emission for scalar
direct-global local memory. Consume only prepared carriers with
`access != nullptr`, `address_space=Default`, `!is_volatile`,
`address.base_kind=PreparedAddressBaseKind::GlobalSymbol`,
`prepared_global_symbol_memory_has_publication_authority(access->address)`,
direct materialization policy, supported 1/2/4/8-byte width, alignment not
larger than width, and an encodable direct-global address+offset sequence.
Include fail-closed RV64-side coverage for missing symbol identity, missing
base-plus-offset, non-default address space, volatile access, unsupported
addressing policy, incomplete extent/range, wrong layout authority, unsupported
width, and large/non-encodable offsets.

## Watchouts

The prepared predicate does not decide volatile, address-space, addressing
policy, width, or target encodability; Step 4 owns those RV64 gates. Keep the
consumer packet limited to direct scalar global-symbol local memory. Do not
admit aggregate byte-storage rows, large offsets without an explicit supported
sequence, string constants, aggregate homes, move-bundle rows, runtime
mismatches, or unsupported-width cases. Do not infer authority from final symbol
spelling or assembly.

## Proof

Ran exactly:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_prepare_stack_layout$' > test_after.log 2>&1`

Result: passed. `test_after.log` is the preserved proof log.
