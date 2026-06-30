Status: Active
Source Idea Path: ideas/open/433_rv64_global_select_pointer_memory_residuals.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Select First Coherent Packet Or Producer Split

# Current Packet

## Just Finished

Completed Step 1: reclassified inherited RV64 global/select/pointer-memory
residual evidence by first owner before selecting an implementation packet.

Bucket table:

| bucket | representative rows | first owner | disposition |
| --- | --- | --- | --- |
| global object-data | `20011019-1` | RV64 object/global data emission | First coherent executable packet: object route reports `unsupported_global_data` with prepared selected object-data contract `status=unsupported_but_coherent`. |
| global memory publication | `930930-1`, `20011019-1`, `20041112-1` | mixed target work after object-data plus producer gaps | Not first; global stores/loads and `store_global_publication` facts depend on object-data and source authority. |
| direct-global select/return | `20011019-1`, `20041112-1` | mixed direct-global select consumer plus terminator/return producer boundary | Not first; direct-global select facts exist, but raw `bir.ret ptr @global` lacks a normal prepared return-authority route. |
| terminator/select publication | `20010329-1`, `20041112-1`, plus select chains in `930930-1`/`20000622-1` | RV64 terminator/select publication or producer split | Not first; separate from global object-data and still fails through `unsupported_terminator_fragment` on representative rows. |
| pointer-value memory | `930930-1` | producer/prepared gap before RV64 lowering | No coherent target packet yet; pointer-value accesses have `layout_authority=unknown` and `range_verdict=unknown_compatible`. |
| producer/prepared gaps | `930930-1`, `20000622-1`, `20041112-1` | producer/prepared facts | Route separately; evidence includes `source_producer=unknown`, inherited `missing_frame_slot_arg_publication`, and missing direct-global return authority. |

Representative evidence:

- `20011019-1`: precise object diagnostic
  `unsupported_global_data: prepared selected object-data contract
  status=unsupported_but_coherent object_label_id=4 object_size_bytes=8
  emitted_byte_count=0 zero_fill_byte_count=0`; also has global load/store and
  direct-global select chains, but object-data fails first.
- `930930-1`: pointer-value memory accesses use `base=pointer_value` with
  `layout_authority=unknown`; also has global `mem` store publication and
  unknown source producers.
- `20010329-1` and `20041112-1`: first object-route owner is
  `unsupported_terminator_fragment`, not pointer/address movement.
- `20041112-1`: raw `bir.ret ptr @global` and prepared global access facts
  require terminator/direct-global return routing separate from object-data.

Artifacts:

- `build/agent_state/433_step1_residual_owner_reclassification/classification.md`
- `build/agent_state/433_step1_residual_owner_reclassification/extracted_evidence.md`

## Suggested Next

Execute Step 2: Select First Coherent Packet Or Producer Split.

Recommended first executable packet:

- Select a narrow RV64 global object-data packet for the
  `unsupported_but_coherent` selected object-data contract represented by
  `20011019-1`.
- Candidate implementation files for the later packet:
  `src/backend/mir/riscv/codegen/prepared_global_memory_emit.cpp`,
  `src/backend/mir/riscv/codegen/object_emission.cpp` only if the object
  writer owns the selected-data diagnostic, and
  `tests/backend/mir/backend_riscv_object_emission_test.cpp`.
- Keep Step 2 selection-only unless the supervisor delegates implementation;
  no target code changes should happen in the selector packet.
- Preserve fail-closed behavior for missing, incoherent, or unsupported object
  data.

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

## Proof

Step 1 delegated backend proof passed and is captured in `test_after.log`:

```sh
{ cmake --build build -j2 && ctest --test-dir build -j2 --output-on-failure -R '^backend_'; } > test_after.log 2>&1 && git diff --check
```
