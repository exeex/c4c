Status: Active
Source Idea Path: ideas/open/442_pointer_value_memory_provenance_publication.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Select Concrete Provenance Or Opaque Policy Packet

# Current Packet

## Just Finished

Completed Step 1: re-audited the four `930930-1` pointer-value memory accesses
against `prepare::prepared_pointer_value_memory_has_proven_authority`.

| access | source pointer | current authority | missing producer fact | classification |
| --- | --- | --- | --- | --- |
| `f:block_4:inst1` load `%t6` | `%t5`, loaded from `%lv.param.reg1`; caller arg3 is `source_base=@mem source_delta=792` | `base=pointer_value`, `pointer=%t5`, `offset=0`, `size=8`, `align=8`, `base_plus_offset=yes`, `layout_authority=unknown`, `range_verdict=unknown_compatible` | Same-module computed-address call-argument provenance is not published onto callee formal `%p.reg1`, local `%lv.param.reg1`, or loaded pointer `%t5`; no complete `@mem` extent/range reaches the access. | Concrete-provenance candidate. |
| `f:logic.rhs.11:inst1` load `%t16` | `%t15`, loaded from `%lv.param.reg1`; same arg3 `@mem+792` source | Same pointer-value shape with unknown layout and unknown-compatible range | Same `%p.reg1`/local/load provenance publication gap. | Concrete-provenance candidate. |
| `f:block_5:inst1` load `%t25` | `%t24`, loaded from `%lv.param.reg1`; same arg3 `@mem+792` source | Same pointer-value shape with unknown layout and unknown-compatible range | Same `%p.reg1`/local/load provenance publication gap; the later store-source use of `%t25` is separate from address authority. | Concrete-provenance candidate. |
| `f:block_5:inst5` store `%t25` | `%t27 = %t26 - 8`; `%t26` loaded from `%lv.param.mr_TR`; caller arg0 is `source_base=@mem source_delta=800` | `base=pointer_value`, `pointer=%t27`, `offset=0`, `size=8`, `align=8`, `base_plus_offset=yes`, `layout_authority=unknown`, `range_verdict=unknown_compatible` | Computed-address provenance is not published onto `%p.mr_TR`/`%lv.param.mr_TR`, not preserved through load `%t26`, and not adjusted through `%t27 = %t26 - 8` to prove `@mem+792`. | Concrete-provenance candidate requiring pointer-delta propagation. |

Classification result: all four rows are concrete-provenance candidates, not
opaque-compatibility candidates. `main` already publishes semantic
computed-address call sources for `@mem+792` and `@mem+800`; the missing piece
is producer publication across the same-module call/formal/local/load chain and
through the one constant pointer delta. No row is intentionally unsupported at
this step.

Artifact:

- `build/agent_state/442_step1_pointer_value_provenance_reaudit/reaudit.md`

## Suggested Next

Execute Step 2: Select Concrete Provenance Or Opaque Policy Packet.

Recommended packet: select a bounded concrete provenance-publication route for
same-module computed-address call arguments into pointer formals. The packet
should define:

- how computed-address call-argument facts become callee formal pointer
  provenance for pointer parameters;
- how that provenance is preserved through local param store/load for
  `%lv.param.reg1` and `%lv.param.mr_TR`;
- whether the constant pointer delta `%t27 = %t26 - 8` is included in the first
  concrete packet or split as the following packet;
- focused producer/prepared tests proving affected pointer-value memory records
  pass `prepare::prepared_pointer_value_memory_has_proven_authority`.

Keep opaque compatibility parked unless the concrete route proves impossible
without testcase-shaped assumptions.

## Watchouts

- Do not weaken `prepare::prepared_pointer_value_memory_has_proven_authority`.
- Do not infer pointer-value memory provenance, layout, object extent, or range
  in RV64 from raw pointer values, integer casts, filenames, function names,
  value names, object labels, or dump shape.
- Treat `layout_authority=unknown` and `range_verdict=UnknownCompatible` as
  fail-closed unless a named and tested opaque-compatibility policy is created.
- Keep store-source/global-memory publication, direct-global
  return/select-chain, and terminator/select publication out of this plan.
- Do not edit RV64 lowering for this producer route.
- Do not key the route to `930930-1`, `reg1`, `mr_TR`, one block label, or one
  dump layout; use same-module call argument and prepared provenance facts as
  authority.

## Proof

Step 1 delegated backend proof passed and is captured in `test_after.log`:

```sh
{ cmake --build build -j2 && ctest --test-dir build -j2 --output-on-failure -R '^backend_'; } > test_after.log 2>&1 && git diff --check
```
