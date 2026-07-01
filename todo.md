Status: Active
Source Idea Path: ideas/open/422_bir_semantic_producer_high_impact_cleanup.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Bucket BIR Producer Families

# Current Packet

## Just Finished

Completed Step 2 for idea 422 by bucketing the 373 current semantic
`lir_to_bir` rows into BIR producer families and classifying implementation
ownership.

Bucket table:

| Bucket | Count | Owning layer | Implementation coherence/risk |
| --- | ---: | --- | --- |
| Local-memory load | 79 | BIR semantic local-memory load producer | Highest count and coherent owner; needs focused probes to split scalar slot, aggregate/member, pointer/provenance, and byval/va_arg-style loads. |
| Local-memory GEP/address | 62 | BIR semantic GEP/address producer | Coherent if narrowed to explicit source object, index, element layout, and pointer provenance facts. |
| Local-memory store | 58 | BIR semantic local-memory store producer | Coherent after probes split scalar stores from aggregate/object-representation writes. |
| Direct-call argument metadata | 52 | BIR semantic direct-call argument/result metadata producer | Coherent call-family owner; needs producer scalar type/source metadata contract. |
| Scalar/local-memory mixed | 49 | BIR scalar plus local-memory boundary | High count but mixed; split before implementation. |
| Memcpy/memset byte/object-representation | 34 | BIR runtime/intrinsic byte/object-representation producer | Coherent family, but memcpy and memset policies and byte-range/object-layout semantics must be split/proven. |
| Alloca-derived storage | 16 | BIR semantic alloca/local stack object producer | Coherent but lower count; depends on explicit alloca object identity, lifetime, layout, and use shape. |
| Scalar-control-flow | 10 | BIR scalar control-flow producer | Separate lower-priority producer family; do not mix with RV64 branch/select consumers. |
| Function-signature | 9 | BIR function signature/type producer | Producer-owned but adjacent to ABI/type admission; separate from ordinary local-memory work. |
| Call-return | 3 | BIR semantic call-return producer | Small call subfamily; separate from direct-call argument metadata. |
| Scalar-binop | 1 | BIR scalar operation producer | Single-row residual; not a first packet. |
| Bootstrap adjacent | 44 | Bootstrap/pre-semantic global/type lowering | Adjacent and excluded from the 373 semantic row buckets. |

Representative rows are captured in the artifact bucket table. Top examples:

- local-memory load: `src/20041124-1.c`, `src/20071219-1.c`,
  `src/991228-1.c`
- local-memory GEP/address: `src/pr44468.c`, `src/pr48571-1.c`,
  `src/pr65956.c`
- local-memory store: `src/20010605-2.c`, `src/920501-5.c`,
  `src/930526-1.c`
- direct-call metadata: `src/pr40022.c`, `src/pr41239.c`, `src/pr49390.c`
- memcpy/memset byte/object-representation: `src/20030408-1.c`,
  `src/pr79354.c`, `src/pr30778.c`, `src/pr51877.c`

Rejected adjacent shapes:

- RV64/MIR recovery from raw shape, value names, testcase names, or target
  fallback inference.
- Prepared/RV64 consumer work such as branch, pointer/address, stack-slot,
  call, or return lowering.
- `unsupported_instruction_fragment` rows from idea 421/420.
- Bootstrap global/type lowering rows.
- F128 helper/runtime rows unless a selected semantic producer bucket proves
  ownership explicitly.

Artifact:

- `build/agent_state/422_step2_bir_producer_buckets/bucket_table.tsv`
- `build/agent_state/422_step2_bir_producer_buckets/rationale.md`

## Suggested Next

Execute Step 3: select the first producer-owned packet. The default
highest-impact candidate is local-memory load, but Step 3 should either select
that packet with a focused representative/contract or explicitly choose a
better coherent packet from GEP, store, direct-call metadata, or memcpy/memset.

## Watchouts

- Current evidence is enough to choose a family, but not enough to implement
  without focused BIR/prepared probes for selected representatives.
- Do not select the mixed scalar/local-memory bucket as-is; split it into a
  sharper local-memory or scalar producer packet first.
- Keep byte/object-representation writes separate from generic scalar local
  stores unless a focused contract explicitly connects them.
- Keep bootstrap rows and `unsupported_instruction_fragment` rows outside this
  semantic producer route.

## Proof

Delegated proof:

```sh
git diff --check
```

Result: passed.
