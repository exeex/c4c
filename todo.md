Status: Active
Source Idea Path: ideas/open/633_aggregate_stack_home_local_memory_policy.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Add Narrow RV64 Stack-Home Consumer Admission

# Current Packet

## Just Finished

Step 4 wired RV64 local-memory emission for the selected byval/sret
stack-home lanes to require
`prepared_stack_home_local_memory_has_authority(...)` before computing stack
offsets for object emission.

The RV64 path now keeps the Step 3 prepared authority predicate as a required
gate while preserving target-specific checks for default address space,
non-volatile accesses, pointer-value bases, 1/2/4/8-byte widths, alignment not
exceeding width, frame bounds, and signed-12-bit offset encodability.

Focused object-emission coverage now proves:
- byval stack-home loads still emit from the prepared stack-home lane when
  explicit byval provenance and range authority are present.
- sret stack-home stores emit only with an explicit stored value identity,
  sret provenance, complete extent, and proven in-bounds range.
- malformed authority fails closed for missing value identity, wrong base
  identity, missing extent authority, unavailable range authority, wrong stack
  object source kind, missing frame-slot/home facts, non-default address space,
  volatile access, non-pointer base, unsupported 16-byte width, and bad target
  alignment/offset shapes.

## Suggested Next

Step 5 evidence packet: rerun the aggregate stack-home focused residual probe
and classify remaining rows after the explicit RV64 byval/sret stack-home
consumer gate.

## Watchouts

The sret object-emission positive fixture now uses a named scalar source with a
prepared register home because the prepared authority contract requires
`stored_value_name`; immediate-only stores remain outside this first sret
stack-home authority lane.

The new gate is deliberately limited to the existing byval load and sret store
helpers. It does not admit outgoing stack arguments, pointer-result rows,
return-destination-only rows, global/string rows, move bundles, F128/16-byte
lanes, large-offset lanes, or plain frame-slot rows through this authority.

## Proof

Proof command:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_riscv_object_emission$' > test_after.log 2>&1`

Result: passed. `test_after.log` contains 1/1 passing
`backend_riscv_object_emission`.
