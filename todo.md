Status: Active
Source Idea Path: ideas/open/630_string_constant_local_memory_policy.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Publish Or Verify Prepared String-Constant Facts

# Current Packet

## Just Finished

Completed Step 3: prepared string-constant local-memory accesses now publish
explicit string bytes/extent authority when the carrier can prove a unique
module `bir::StringConstant` identity, nonempty byte payload, default address
space, and an in-bounds requested range.

Implementation details:

- Added `bir::MemoryLayoutAuthorityKind::StringConstantBytes` and the prepared
  spelling `string_constant_bytes`.
- Added `prepared_string_constant_local_memory_has_authority(...)` as the
  string-specific prepared carrier predicate. Generic global and pointer-value
  predicates do not accept the new string authority.
- `build_direct_symbol_backed_address(...)` now resolves
  `PreparedAddressBaseKind::StringConstant` by a unique `bir::StringConstant`
  text identity instead of treating strings as raw compatibility globals.
- `publish_string_constant_local_memory_authority(...)` joins prepared string
  accesses to `bir::StringConstant::bytes`, publishes complete object extent,
  proves the requested range, and sets `StringConstantBytes` only after the
  range is proven in bounds.
- Focused `backend_prepare_stack_layout` coverage exercises a positive
  default-address-space 8-byte string access and fail-closed cases for missing
  string identity, empty bytes, out-of-bounds range, non-default address space,
  ambiguous string identity, empty base identity, and unrelated global/pointer
  prepared address bases.

## Suggested Next

Execute the smallest Step 4 RV64 object-route consumer packet: admit only
`PreparedAddressBaseKind::StringConstant` local-memory loads whose prepared
carrier satisfies `prepared_string_constant_local_memory_has_authority(...)`,
default address space, nonvolatile access, 8-byte pointer width/alignment,
base-plus-offset form, nonempty prepared string label, signed-12-bit offset, and
in-bounds string bytes/extent. Keep stores, non-default address spaces,
out-of-bounds ranges, missing/ambiguous string identity, and unrelated
global/frame/pointer cases rejected.

## Watchouts

Do not widen RV64 admission through frame-slot, generic global, pointer-value,
or filename/literal-spelling inference. `StringConstantBytes` is intentionally
not accepted by generic global or pointer-value authority predicates. The Step 4
consumer should require this string-specific prepared authority and should not
modify GCC torture expectations, unsupported markers, allowlists, or final
assembly expectations.

## Proof

Ran the delegated proof exactly:

`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_prepare_stack_layout$' > test_after.log 2>&1`

Result: build succeeded and `backend_prepare_stack_layout` passed. Proof log:
`test_after.log`.
