Status: Active
Source Idea Path: ideas/open/422_bir_semantic_producer_high_impact_cleanup.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Select The First Producer-Owned Packet

# Current Packet

## Just Finished

Completed Step 3 for idea 422 by selecting the first producer-owned packet
from the bucketed evidence and recording the focused implementation contract
and probe requirements.

Selected first packet:

| Packet | Count | Owning layer | Disposition |
| --- | ---: | --- | --- |
| Local-memory load | 79 | BIR semantic local-memory load producer | Selected as the highest-impact coherent ordinary C producer family. Split/activate as a narrower implementation idea rather than implementing directly under idea 422. |

Representative selected rows:

- `src/20041124-1.c`
- `src/20071219-1.c`
- `src/991228-1.c`
- `src/multi-ix.c`
- `src/pr22098-1.c`

Required BIR/prepared contract for the future implementation packet:

- admit only semantic local-memory load rows with explicit load operation
  identity, source memory/address value, local object or frame-slot identity,
  object layout, size, alignment, access range, scalar result type, and
  non-bootstrap ordinary C origin;
- prove source/destination identity through BIR producer facts, not RV64 target
  fallback, testcase names, raw dump order, value-name patterns, final homes, or
  expectation/allowlist changes;
- start with one scalar local-object load shape; keep aggregate/member,
  GEP-derived, byval/va_arg, pointer/provenance, volatile/atomic, complex,
  vector, and F128 shapes fail-closed until separately owned.

Rejected adjacent buckets/shapes for the first packet:

- local-memory GEP/address rows;
- local-memory stores and byte/object-representation writes;
- direct-call argument/result metadata;
- alloca-derived storage and dynamic stack object lifetime rows;
- memcpy/memset runtime byte-range rows;
- mixed scalar/local-memory rows that cannot be reduced to the selected scalar
  local-load contract;
- bootstrap global/type rows;
- prepared/RV64 materialization, branch/select, stack-home, call, return, or
  target fallback recovery.

Artifact:

- `build/agent_state/422_step3_first_producer_packet/decision.md`

## Suggested Next

Execute Step 4 for idea 422: record residual disposition and close readiness.
Recommended lifecycle outcome is to close or retire idea 422 as a planning and
selection runbook after routing the selected implementation to a narrower
producer-owned idea such as `BIR semantic local-memory scalar load producer`.

## Watchouts

- Do not implement the selected load packet in idea 422 without plan-owner
  lifecycle action; Step 3 recommends a split/narrow implementation idea.
- The next implementation idea should first collect focused probes for the
  representative local-memory load rows and prove scalar load identity plus
  source object/layout facts.
- Keep GEP/store/direct-call/memcpy/allocation/runtime/consumer buckets outside
  the first local-memory load packet.

## Proof

Delegated proof:

```sh
git diff --check
```

Result: passed.
