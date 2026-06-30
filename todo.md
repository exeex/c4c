Status: Active
Source Idea Path: ideas/open/433_rv64_global_select_pointer_memory_residuals.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Residual Disposition And Close Readiness

# Current Packet

## Just Finished

Completed Step 3: implemented the selected narrow RV64 global object-data
packet for coherent selected object-data contracts.

Implementation summary:

- Added RV64 object-route emission for selected `unsupported_but_coherent`
  global object data when prepared facts and prepared global facts authorize a
  pointer-sized zero object. The route emits the object through `.bss` using the
  prepared selected object-data size/alignment.
- Added RV64 object-route emission for selected pointer global object data
  initialized from an explicit prepared link-name symbol. The route emits an
  8-byte data object with an `R_RISCV_64` relocation to the prepared target
  symbol.
- Kept missing/incoherent selected object-data contracts fail-closed through the
  existing prepared contract verifier diagnostic.
- Added focused backend coverage for accepted selected symbol-pointer data,
  accepted selected zero pointer data, and rejected unsupported selected raw
  pointer data.

Files changed:

- `src/backend/mir/riscv/codegen/object_emission.cpp`
- `tests/backend/mir/backend_riscv_object_emission_test.cpp`
- `todo.md`

Representative probe:

- `20011019-1` prepared probe succeeded.
- `20011019-1` RV64 object probe now succeeds and no longer stops at
  `unsupported_global_data: prepared selected object-data contract
  status=unsupported_but_coherent`.

Artifacts:

- `build/agent_state/433_step3_global_object_data/summary.txt`
- `build/agent_state/433_step3_global_object_data/20011019-1.prepared.out`
- `build/agent_state/433_step3_global_object_data/20011019-1.object.err`

## Suggested Next

Execute Step 4: Residual Disposition And Close Readiness.

Recommended packet:

- Re-probe the residual bucket rows from the active source idea and classify
  what remains after the selected object-data packet.
- Record whether remaining failures belong to global memory publication,
  direct-global select/return, terminator/select publication, pointer-value
  memory, or producer/prepared gaps.
- Decide whether this source idea is close-ready or needs a separate coherent
  Step 4 target packet/follow-up idea.

Suggested proof command:

```sh
{ cmake --build build -j2 && ctest --test-dir build -j2 --output-on-failure -R '^backend_'; } > test_after.log 2>&1 && git diff --check
```

## Watchouts

- Do not reopen bounded `inttoptr`/`ptrtoint` pointer-cast movement from idea
  429.
- Do not infer global object identity, symbol addressability, relocation base,
  memory layout, select-chain roots, or return pointer authority in RV64 from
  raw BIR or testcase shape.
- Keep aggregate ABI, F128, scalar FP, div/rem, and unrelated call-publication
  work out of scope.
- If evidence shows a producer/prepared gap, route it to a separate source idea
  instead of bypassing it in RV64.
- Do not infer global object identity, byte layout, zero-fill, relocation base,
  or symbol addressability from raw BIR, testcase filenames, function names, or
  object label numbers.
- The implemented route is intentionally limited to prepared selected
  object-data facts plus explicit prepared global initializer facts; do not
  broaden Step 4 into global load/store publication, direct-global
  select/return, pointer-value memory, terminator/select publication, or
  producer-gap repairs without fresh representative evidence.

## Proof

Step 3 delegated backend proof passed and is captured in `test_after.log`:

```sh
{ cmake --build build -j2 && ctest --test-dir build -j2 --output-on-failure -R '^backend_'; } > test_after.log 2>&1 && git diff --check
```
