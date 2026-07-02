# Current Packet

Status: Active
Source Idea Path: ideas/open/531_bir_memory_provenance_header_readiness.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Split Only A Proven Safe Public Support Boundary

## Just Finished

Completed Step 2 of `plan.md` as a behavior-preserving declaration-surface
split. Added public `src/backend/bir/bir_memory_provenance.hpp` containing only
the proven `MemoryAccessProvenance` support cluster:
`MemoryProvenanceBaseIdentityKind`, `MemoryObjectExtentCompleteness`,
`MemoryLayoutAuthorityKind`, `MemoryRangeVerdict`,
`MemoryDynamicArrayRangeVerdict`, `MemoryProvenanceBaseIdentity`,
`MemoryObjectExtent`, `MemoryByteRange`, `make_memory_byte_range`,
`MemoryDynamicArrayFacts`, `MemoryAccessProvenance`,
`prove_memory_dynamic_array_range`, and
`prove_memory_access_requested_range`.

`src/backend/bir/bir.hpp` now includes the new header at the original
declaration boundary after `Value` and text-id prerequisites are complete.
`bir.hpp` remains the compatibility aggregator. No route3, lowering, prealloc,
MIR, object emission, or test include replacements were made. Enum values,
record fields, layout, optionality, range/verdict logic, and behavior were left
unchanged.

## Suggested Next

Delegate Step 3 as a direct-include probe packet: test whether
`src/backend/bir/bir_memory_provenance.hpp` can be included directly by any
selected public consumers without depending on `bir.hpp`, and either make only
independently buildable include replacements or record the exact
complete-type/include-cycle blocker and park direct include reduction.

## Watchouts

- The new header is aggregator-only at the end of Step 2. It depends on a
  complete `bir::Value` because `MemoryProvenanceBaseIdentity` and
  `MemoryDynamicArrayFacts` store `Value` by value.
- Direct include replacement still belongs to Step 3 only after compile probes
  prove a consumer already has `Value` and text-id prerequisites without
  relying on `bir.hpp`.
- Keep `MemoryAddress`, route3 records, static GEP authority records,
  LIR-to-BIR lowering-private records, prealloc/MIR consumers, and tests on
  `bir.hpp` unless the Step 3 proof shows a narrower include is independently
  buildable.
- Do not change provenance authority verdicts, storage semantics, enum values,
  record layout, optionality, lookup behavior, lowering behavior, or idea 422
  producer behavior under this route.

## Proof

Ran `cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_' > test_after.log 2>&1`.
The first build attempt stopped when `cc1plus` was killed while compiling
`backend_aarch64_instruction_dispatch_test.cpp`, with no include/type error.
Reran the exact delegated proof after the partial build; it passed with 345
backend tests in `test_after.log`.
