Status: Active
Source Idea Path: ideas/open/422_bir_semantic_producer_high_impact_cleanup.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Residual Disposition And Close Readiness

# Current Packet

## Just Finished

Completed Step 4 for idea 422 by recording residual disposition and
close/handoff readiness after selecting the local-memory load packet.

Disposition:

- idea 422 is complete as a high-impact BIR semantic producer cleanup planning
  runbook;
- it should close or retire after plan-owner lifecycle action creates or
  activates a narrower implementation idea;
- the exact proposed follow-up title is `BIR semantic local-memory scalar load
  producer`;
- implementation should not happen directly inside idea 422 because the
  selected load family needs focused probes, a scalar local-object load
  contract, and fail-closed tests before code changes.

Selected follow-up scope:

- first owner: BIR semantic local-memory load producer;
- selected bucket: local-memory load, 79 rows;
- representative rows: `src/20041124-1.c`, `src/20071219-1.c`,
  `src/991228-1.c`, `src/multi-ix.c`, `src/pr22098-1.c`;
- required future contract: semantic load identity, source memory/address
  value, local object or frame-slot identity, layout, size, alignment, access
  range, scalar result type, and ordinary C origin must be explicit producer
  facts.

Residual producer buckets and owners:

| Residual bucket | Count | First owner | Disposition |
| --- | ---: | --- | --- |
| Local-memory load | 79 | BIR semantic local-memory load producer | Selected follow-up; split to scalar local-load implementation idea. |
| Local-memory GEP/address | 62 | BIR semantic local-memory GEP/address producer | Separate producer idea after object/index/layout/provenance contract. |
| Local-memory store | 58 | BIR semantic local-memory store producer | Separate producer idea after scalar-store versus byte/object-representation split. |
| Direct-call argument metadata | 52 | BIR semantic direct-call argument/result metadata producer | Separate call-family producer idea. |
| Scalar/local-memory mixed | 49 | BIR scalar plus local-memory boundary | Split before implementation. |
| Memcpy/memset byte/object-representation | 34 | BIR runtime/intrinsic byte/object-representation producer | Separate runtime/object-representation producer work. |
| Alloca-derived storage | 16 | BIR semantic alloca/local stack object producer | Separate alloca identity/lifetime/layout producer work. |
| Scalar-control-flow | 10 | BIR scalar control-flow producer | Separate lower-priority producer work. |
| Function-signature | 9 | BIR function signature/type producer | Separate ABI/type admission producer work. |
| Call-return | 3 | BIR semantic call-return producer | Separate call-return producer work. |
| Scalar-binop | 1 | BIR scalar operation producer | Single-row residual; not first packet. |
| Bootstrap adjacent | 44 | Bootstrap/pre-semantic global/type lowering | Adjacent pre-semantic owner outside the selected ordinary C packet. |

Artifact:

- `build/agent_state/422_step4_residual_disposition/disposition.md`

## Suggested Next

Plan-owner lifecycle packet: close or retire idea 422 as completed
planning/selection work and create or activate the narrower `BIR semantic
local-memory scalar load producer` implementation idea.

## Watchouts

- No RV64 lowering should recover missing semantic producer facts from raw BIR
  shape, value names, testcase names, final homes, or target fallback inference.
- Do not broaden idea 422 into generic semantic producer implementation; the
  selected local-memory load packet needs a dedicated source idea/runbook.
- Keep GEP, store, direct-call, memcpy/memset, alloca, scalar, function
  signature, call-return, and bootstrap buckets as separate first-owner
  boundaries.

## Proof

Delegated proof:

```sh
git diff --check
```

Result: passed.
