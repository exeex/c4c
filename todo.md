Status: Active
Source Idea Path: ideas/open/348_aarch64_indexed_aggregate_address_writeback.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Repair Selected Address And Writeback Propagation

# Current Packet

## Just Finished

Step 3 completed a focused selected FP store/readback publication slice for
dynamic selected aggregate operations.

Implementation completed in this slice:

- Store-owned global value publication can now materialize FPR-backed selected
  values before a selected store, instead of only publishing GPR-backed values.
- The FP publication helper now handles same-block `select` producers and
  nested global-load fallback arms. It materializes the false arm into the
  destination, then the current true arm into a scratch immediately before the
  compare/`fcsel`, so nested selected readback cannot overwrite an outer
  select arm before it is used.
- Direct-global selected FP call arguments now publish into their prepared FPR
  home before call-boundary moves. This repairs the `00195` final `printf`
  readback path, where the call previously consumed stale FPRs instead of the
  dynamically selected struct-member values.
- Focused dispatch coverage now proves an `f64` select feeding a selected
  global store publishes into the prepared FPR home before the store consumes
  it, and an `f64` selected global readback publishes before a call argument
  ABI move.

Representative status strictly improved versus `test_before.log`: `00130`,
`00187`, and `00195` pass. The remaining red representatives are `00176`,
`00181`, and `00182`.

## Suggested Next

Continue Step 3 on the remaining dynamic selected aggregate writeback
representatives, starting with the smallest first-bad-fact boundary among
`00176`, `00181`, and `00182`. Keep the next packet scoped to selected
aggregate address/writeback publication; do not widen into unrelated scalar
lowering or expectation policy.

## Watchouts

- Keep implementation scoped to active focused idea 348, not the parked
  umbrella inventory idea.
- Do not special-case c-testsuite filenames, source symbols, byte offsets,
  temporary registers, or emitted instruction sequences.
- The FP select publication order matters: nested selects must publish the
  false arm first and refresh the true-arm scratch immediately before `fcsel`.
- The remaining red external tests in `test_after.log` are `00176`, `00181`,
  and `00182`; `00130`, `00187`, and `00195` pass.

## Proof

Executor proof was run exactly into `test_after.log`:

```sh
cmake --build --preset default && ctest --test-dir build --output-on-failure -R 'backend_aarch64_(memory_operand_records|prepared_memory_operand_records|memory_operand_contract)|c_testsuite_aarch64_backend_src_001(30|76|81|82|87|95)_c' | tee test_after.log
```

Result: 6/9 passed, improving over the 5/9 `test_before.log` baseline with no
new failing tests in the delegated subset. Focused dispatch coverage was also
built and run via `backend_aarch64_instruction_dispatch` before the delegated
proof.

Supervisor acceptance validation:

- Regression guard PASS: delegated subset improved from 5/9 to 6/9, resolving
  `00195`, with no new failures.
- Broader backend validation PASS: `ctest --test-dir build -j
  --output-on-failure -R '^backend_'` passed 141/141.
