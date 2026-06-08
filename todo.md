Status: Active
Source Idea Path: ideas/open/126_aarch64_memory_va_list_field_local_owner.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Focused Proof and Broader Check Decision

# Current Packet

## Just Finished

Completed Step 5 of `plan.md`: ran the delegated focused build/test proof and
recorded the close-decision status for the AArch64-local `va_list` field memory
owner cleanup.

The focused proof passed without expectation downgrades. It covers the selected
AArch64 memory operand records, prepared memory operand records, memory
operand contract, prepared frame stack-call contract, variadic aggregate
overflow byte-copy route, prepared BIR 00204 handoff dumps, AArch64
publication dump, and `c_testsuite` 00204 backend regression.

The runbook appears exhausted from the executor side: Steps 1 through 5 are
complete, implementation is already grouped behind the private/file-local owner
surface, Step 4 found no reconnect/fallback defect, and this Step 5 focused
proof is green. This is ready for the supervisor to route to the plan owner for
the lifecycle close decision.

## Suggested Next

Supervisor close-decision packet: pass the completed `todo.md` state, focused
proof result, broader backend pass, and accepted full-suite baseline to the
plan owner to decide whether to close or otherwise retire the active runbook.

## Watchouts

- Coverage weakness: the delegated subset proves the route mostly through
  integration and prepared-dump expectations; there is no narrow unit test that
  directly constructs a `BlockLoweringContext` and asserts
  `VaListFieldMemoryOwner` load/store/cursor-update selection and fallback
  behavior by owner method.
- This remains an AArch64-local ownership cleanup. Shared prepared variadic
  facts are still consumed, not recomputed or moved.
- Existing supervisor broader backend pass from the Step 3 code slice is
  already recorded as green: `ctest --test-dir build -j --output-on-failure -R
  '^backend_'`, all 179 backend tests passed.
- Existing accepted full-suite baseline for commit `85c17cfed` is recorded in
  `test_baseline.log`: baseline commit
  `85c17cfedbe42ec3e6c29d9e5c28a91da71ee099`, subject `Group AArch64 va_list
  field memory owner`, `<full-suite>`, all 3427 tests passed.

## Proof

Delegated proof for Step 5:

```sh
cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_aarch64_(memory_operand_records|prepared_memory_operand_records|memory_operand_contract)|backend_prepare_frame_stack_call_contract|backend_codegen_route_aarch64_variadic_aggregate_overflow_byte_copy|backend_cli_dump_prepared_bir_00204_stdarg_prepared_handoff(_aarch64_publication)?|c_testsuite_aarch64_backend_src_00204_c)$' > test_after.log 2>&1
```

Result: build succeeded (`ninja: no work to do`) and all 8 focused tests passed.
Proof log: `test_after.log`.

Supervisor broader acceptance retained from the Step 3 code slice:

```sh
ctest --test-dir build -j --output-on-failure -R '^backend_'
```

Result: all 179 backend tests passed.

Accepted full-suite baseline retained for commit `85c17cfed`:

```text
Baseline Commit: 85c17cfedbe42ec3e6c29d9e5c28a91da71ee099
Baseline Subject: Group AArch64 va_list field memory owner
Baseline Regex: <full-suite>
```

Result: all 3427 tests passed.
