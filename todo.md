Status: Active
Source Idea Path: ideas/open/237_aarch64_binary128_softfloat_lowering.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Inventory Current F128 Prepared Coverage

# Current Packet

## Just Finished

Step 1 inventory found the first missing AArch64 binary128 prepared fact
family: there is no F128 full-width carrier authority parallel to the existing
I128 carrier path. Added dispatch-level fail-closed coverage for an F128
frame-slot load with complete 16-byte memory/address/storage facts, so it now
reports `missing_prepared_f128_carrier` instead of falling through to generic
memory selection. Existing prepared memory, scalar-cast, and I128 dispatch
behavior is unchanged by the proof subset.

## Suggested Next

Delegate Step 2 to add structured F128 carrier facts in prealloc for full-width
16-byte value homes, starting with memory/load result carrier identity that can
satisfy the new fail-closed dispatch guard.

## Watchouts

- Do not claim binary128 progress through scalar `F64` lowering.
- Do not add testcase-shaped helper shortcuts or weaken unsupported
  expectations.
- Keep atomic, intrinsic, and inline-assembly AArch64 routes out of this plan.
- The new guard covers F128 local load/store memory transport only; helper-call,
  arithmetic, casts, constants, and selected F128 machine records remain
  unsupported until their prepared facts exist.

## Proof

`set -o pipefail; { cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_aarch64_prepared_memory_operand_records|backend_aarch64_prepared_scalar_cast_records|backend_aarch64_instruction_dispatch)$'; } 2>&1 | tee test_after.log`

Passed. Proof log: `test_after.log`.
