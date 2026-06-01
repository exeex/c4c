Status: Active
Source Idea Path: ideas/open/75_shared_aggregate_transport_plan_probe.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Implement Shared Transport Authority If Required

# Current Packet

## Just Finished

Step 3 of `plan.md` migrated the AArch64 byval register-lane call-argument
publication source path to consume `PreparedCallArgumentPlan::aggregate_transport`.

- `calls.cpp` now requires a `ByvalRegisterLanes` aggregate transport plan for
  byval register-lane payload size, source frame slot/offset, required chunks,
  and lane coverage before constructing the prepared source memory operand.
- The small register-lane and stack-lane publication paths no longer use
  `source_selection` as lane/source/chunk authority; complete legacy
  `ByvalRegisterLane` source-selection facts without aggregate transport fail
  closed.
- The large indirect byval address path remains on its existing selected-source
  authority for payloads larger than the register-lane publication limit.
- Focused AArch64 MIR dispatch coverage now publishes aggregate transport for
  preserved-output success cases and checks that a complete old selection
  without aggregate transport does not lower the register-lane publication.

## Suggested Next

Next coherent Step 3 packet: review whether any remaining AArch64 aggregate
consumer path should also read shared aggregate transport before moving Step 3
to broader review/closure.

## Watchouts

- The migrated helper validates contiguous required chunk and lane coverage for
  up to 16-byte register-lane payloads; larger byval aggregates still use the
  existing indirect-address path.
- Keep variadic aggregate `va_arg` plans separate.
- Do not weaken aggregate testcase expectations or add named-case shortcuts if
  further consumer migration is needed.

## Proof

Command:

```bash
cmake --build --preset default > test_after.log && ctest --test-dir build -j --output-on-failure -R '^(backend_aarch64_instruction_dispatch|backend_prepare_frame_stack_call_contract|backend_prepared_printer)$' >> test_after.log
```

Result: passed, 3/3 selected tests. Proof log: `test_after.log`.
