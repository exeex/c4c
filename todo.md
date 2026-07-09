Status: Active
Source Idea Path: ideas/open/633_aggregate_stack_home_local_memory_policy.md
Source Plan Path: plan.md
Current Step ID: 10
Current Step Title: Repair The Verified Residual Stack-Home Route

# Current Packet

## Just Finished

Step 10 repaired the verified residual sret stack-home local-memory route.
`prepared_stack_home_local_memory_has_authority(..., SretParam)` now validates
the selected access against the proven return-pointee extent and requested
range instead of conflating that extent with the 8-byte `%ret.sret` pointer
home. Byval authority still requires the provenance extent to match the stack
object extent.

Focused prepared-layer coverage now proves sret authority for 6-, 16-, and
24-byte return-pointee extents while the `%ret.sret` pointer home remains an
8-byte stack slot. Negative coverage keeps missing value homes, missing frame
slots, wrong source kind, stale source identity, missing sret provenance,
incomplete extent, out-of-range lanes, malformed requested ranges, non-default
address spaces, volatile accesses, non-pointer bases, and 16-byte local-memory
rows fail-closed.

Focused RV64 object-emission coverage now proves explicit-authority sret
integer store lanes for I8, I16, I32, and I64 shapes matching the
`950628-1`, `pr30185`, and `20020215-1` residual route. Existing sret F32 and
malformed-route coverage remains intact.

## Suggested Next

Run Step 11 reclassification with the same aggregate stack-home residual probe
used by Steps 5 and 8. Confirm whether `950628-1`, `pr30185`, `20020215-1`,
and related sret rows move past `unsupported_local_memory_access`, then
classify any remaining rows into stack-home local-memory, existing open ideas,
or new durable residual owners.

## Watchouts

The repair is role-specific: byval extent matching was preserved, while sret
uses the return-pointee extent from prepared memory provenance. Do not broaden
Step 11 conclusions into later call ABI, call-instruction, branch stack-load,
move-bundle, aggregate global-data, F128/16-byte, runtime,
pointer-loaded-from-global, or mixed local/global rows.

The exact proof initially hit an unrelated `cc1plus` kill while compiling
`backend_aarch64_instruction_dispatch_test`; a single-target `-j1` build of
that existing test binary was used only to relieve build pressure, then the
delegated proof command was rerun exactly and passed.

## Proof

Delegated proof command:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_prepare_stack_layout|backend_riscv_object_emission)$' > test_after.log 2>&1`

Result: passed, 2/2 selected tests.

Proof log: `test_after.log`.

Extra check: `git diff --check` passed.
