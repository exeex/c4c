Status: Active
Source Idea Path: ideas/open/433_rv64_global_select_pointer_memory_residuals.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Execute Or Route Residual Packet

# Current Packet

## Just Finished

Completed Step 2: selected the first coherent executable packet for the RV64
global/select/pointer residual plan.

Selected first packet: narrow RV64 global object-data support for coherent
selected object-data contracts.

Selection evidence:

- Representative row: `20011019-1`.
- First object-route owner:
  `unsupported_global_data: prepared selected object-data contract
  status=unsupported_but_coherent object_label_id=4 object_size_bytes=8
  emitted_byte_count=0 zero_fill_byte_count=0`.
- The packet is coherent because object-data fails before later global
  load/store publication, direct-global select-chain, terminator, or
  pointer-value memory residuals.
- No producer split is needed for the first packet; producer/prepared gaps
  remain separate for later residual routing.

Artifacts:

- `build/agent_state/433_step2_first_packet_selection/selection.md`

## Suggested Next

Execute Step 3: Execute Or Route Residual Packet.

First implementation packet:

- Owned files:
  `src/backend/mir/riscv/codegen/prepared_global_memory_emit.cpp`,
  `src/backend/mir/riscv/codegen/object_emission.cpp`,
  `tests/backend/mir/backend_riscv_object_emission_test.cpp`, `todo.md`,
  `test_after.log`, and `build/agent_state/433_step3_global_object_data/` if
  implementation probes are needed.
- Implement only coherent RV64 selected global object-data emission using
  prepared selected object-data facts as authority.
- Add focused backend coverage for accepted coherent selected object-data and
  fail-closed missing/incoherent/unsupported object-data contracts.
- Preserve fail-closed behavior for unsupported selected data forms.

Focused proof command for Step 3:

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
- Do not bundle global load/store publication, direct-global select/return,
  pointer-value memory, terminator/select publication, or producer-gap repairs
  into the first global object-data packet.
- Do not infer global object identity, byte layout, zero-fill, relocation base,
  or symbol addressability from raw BIR, testcase filenames, function names, or
  object label numbers.
- Do not claim the full `20011019-1` row is solved by object-data support; it
  still contains later global memory/direct-global select residuals.

## Proof

Step 2 delegated backend proof passed and is captured in `test_after.log`:

```sh
{ cmake --build build -j2 && ctest --test-dir build -j2 --output-on-failure -R '^backend_'; } > test_after.log 2>&1 && git diff --check
```
