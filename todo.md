Status: Active
Source Idea Path: ideas/open/442_pointer_value_memory_provenance_publication.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Rebuild Residual Provenance Evidence

# Current Packet

## Just Finished

Step 1 - Rebuild Residual Provenance Evidence completed for
`tests/c/external/gcc_torture/src/930930-1.c` after ideas 443, 444, and 445.

Commands and evidence sources used:

- `./build/c4cll -I tests/c/external/gcc_torture --target riscv64-linux-gnu
  --dump-prepared-bir tests/c/external/gcc_torture/src/930930-1.c`
  redirected to `/tmp/930930-1.step1.prepared.out` and
  `/tmp/930930-1.step1.prepared.err`.
- `build/rv64_gcc_c_torture_backend/src_930930-1.c/case.log`
- `build/agent_state/442_step1_pointer_value_provenance_reaudit/reaudit.md`
- `build/agent_state/442_step3_concrete_call_arg_provenance/summary.md`
- `build/agent_state/443_step4_residual_disposition/classification.md`
- `build/agent_state/443_step4_residual_disposition/evidence_snippets.txt`
- `build/agent_state/444_step1_no_external_caller_audit/audit.md`
- `build/agent_state/444_step2_no_external_caller_contract/contract.md`
- `build/agent_state/445_step1_metadata_source_classification/classification.md`
- `build/agent_state/445_step2_source_contract_rejection/rejection.md`
- `tests/c/external/gcc_torture/src/930930-1.c`

Closed prerequisite result consumed:

- Idea 443 added the formal pointer authority carrier and preserves
  `InternalOnly` for static/internal callees.
- Idea 444 defined the `NoExternalCaller` producer contract but found no
  current producer packet.
- Idea 445 rejected every current non-internal metadata source for
  `FormalPointerAuthorityKind::NoExternalCaller`.
- Therefore `NoExternalCaller` remains unpopulated, `930930-1::f` remains
  `FormalPointerAuthorityKind::Unknown`, and external-linkage formal
  provenance must not be inferred from observed same-module direct calls or
  call-argument computed-address dumps.

Current top-level diagnostic:

```text
unsupported_instruction_fragment: BIR instruction requires unsupported RV64 object lowering
```

Current call/formal evidence:

- Source `930930-1.c` defines external-linkage K&R function `f` and calls it as
  `f (mem + 100, mem + 6, mem + 8, mem + 99, mem + 99)`.
- Prepared call-argument sources remain visible:
  arg0 `source_base=@mem source_delta=800`, arg1 `@mem+48`,
  arg2 `@mem+64`, arg3 `@mem+792`, and arg4 `@mem+792`.
- Formal homes/storage remain register-backed for `%p.mr_TR` and `%p.reg1`,
  and local slots for `%lv.param.mr_TR`/`%lv.param.reg1` have size 8/align 8.
- Those call observations are not complete authority for external-linkage
  formals after ideas 443/444/445.

Residual row table:

| Row | Access evidence | Layout/range | Formal authority | Object extent/provenance source | Current classification |
| --- | --- | --- | --- | --- | --- |
| `reg1` load A | `/tmp/930930-1.step1.prepared.out:568`: `f:block_4:inst1`, `base=pointer_value`, `result=%t6`, `pointer=%t5`; `%t5` loaded from `%lv.param.reg1`; arg3 call source is `@mem+792`. | `layout_authority=unknown`, `range_verdict=unknown_compatible`, `size=8`, `align=8`. | `f` is external-linkage, so `%p.reg1` is `Unknown`; `NoExternalCaller` is unpopulated. | Potential semantic object would be `@mem`, source extent 100 pointer elements / 800 bytes from source, but no prepared formal provenance or object extent is attached to the pointer value. | Intentionally unsupported under current model. Do not infer from same-module callsite. |
| `reg1` load B | `/tmp/930930-1.step1.prepared.out:570`: `f:logic.rhs.11:inst1`, `base=pointer_value`, `result=%t16`, `pointer=%t15`; `%t15` loaded from `%lv.param.reg1`; arg3 call source is `@mem+792`. | `layout_authority=unknown`, `range_verdict=unknown_compatible`, `size=8`, `align=8`. | Same external-linkage `Unknown` formal authority for `%p.reg1`. | Same potential `@mem` source, but unavailable to the pointer-value row without accepted formal authority. | Intentionally unsupported under current model. |
| `reg1` load C | `/tmp/930930-1.step1.prepared.out:572`: `f:block_5:inst1`, `base=pointer_value`, `result=%t25`, `pointer=%t24`; `%t24` loaded from `%lv.param.reg1`; arg3 call source is `@mem+792`. | `layout_authority=unknown`, `range_verdict=unknown_compatible`, `size=8`, `align=8`. | Same external-linkage `Unknown` formal authority for `%p.reg1`. | Same potential `@mem` source, but unavailable to the pointer-value row without accepted formal authority. | Intentionally unsupported under current model. |
| `%mr_TR - 8` pointer-delta store | `/tmp/930930-1.step1.prepared.out:575`: `f:block_5:inst5`, `base=pointer_value`, `stored=%t25`, `pointer=%t27`; `%t27` is stored after `%t26` load from `%lv.param.mr_TR` and pointer decrement; arg0 call source is `@mem+800`. | `layout_authority=unknown`, `range_verdict=unknown_compatible`, `size=8`, `align=8`. | `%p.mr_TR` is also external-linkage `Unknown`; no base formal provenance exists. | Potential concrete target would be `@mem+792` after subtracting 8 from `@mem+800`, but neither base formal authority nor pointer-delta propagation is available. | Pointer-delta-later-work only after base formal authority exists; currently fail-closed. |
| Frame-slot param/local rows | `/tmp/930930-1.step1.prepared.out:562-567`, `:569`, `:571`, `:573-574`, `:576-579` use `base=frame_slot` for parameter local slots and temporaries. | `layout_authority=unknown`, `range_verdict=proven_in_bounds`, `size=8`, `align=8`. | Not formal pointer provenance publication rows; they are local/frame-slot storage facts. | Local slot/object extent is visible as 8-byte pointer slots; not the pointee object extent. | Unrelated to pointer-value memory provenance except as carrier loads/stores. |
| Global `@mem+792` store | `/tmp/930930-1.step1.prepared.out:581`: `base=global_symbol`, `symbol=mem`, `offset=792`, `size=8`, `align=8`. | Now `layout_authority=byte_storage_aggregate`, `range_verdict=proven_in_bounds`. | Not a formal pointer row. | Concrete global object `@mem`; source declares `ptr_t mem[100]`. | Unrelated global-layout progress; does not authorize external-linkage formal pointer values. |

Step 1 disposition:

- Concrete provenance: none currently available for the residual pointer-value
  rows because base formal authority is unavailable.
- Possible opaque compatibility: no row selected yet; Step 2 may only accept a
  named opaque policy if it is semantic and does not weaken
  `prepared_pointer_value_memory_has_proven_authority`.
- Intentionally unsupported: the three `reg1` pointer-value loads remain
  fail-closed under the current compiler model.
- Pointer-delta-later-work: the `%mr_TR - 8` store remains split until base
  formal authority and pointer-delta propagation are both real.
- Unrelated: frame-slot carrier rows and the now-authorized `@mem+792`
  global-symbol store must not be folded into this route.

## Suggested Next

Delegate Step 2 - Decide Concrete, Opaque, Or Unsupported Policy. The starting
point should be fail-closed/unsupported for external-linkage `930930-1::f`
unless a narrow opaque policy can be justified without target-side inference.

## Watchouts

Do not resurrect the pre-443 concrete-callsite route for external-linkage
`930930-1::f`. Ideas 443, 444, and 445 closed by preserving
external-linkage/no-proof fail-closed behavior: `NoExternalCaller` remains
unpopulated. Do not infer pointer-value memory authority in RV64 target
lowering, and do not treat current `@mem` global-layout authority as formal
pointer provenance.

## Proof

`git diff --check -- todo.md`
