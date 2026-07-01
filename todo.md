Status: Active
Source Idea Path: ideas/open/485_bir_local_array_address_derivation_index_range_authority_carrier.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Residual Disposition And Close Readiness

# Current Packet

## Just Finished

Completed Step 4 for idea 485 by recording residual disposition after the
local array carrier records landed.

Close-readiness classification:

| Scope | Disposition |
| --- | --- |
| Source-object carrier records | Complete for static local arrays lowered from explicit local array `alloca` evidence |
| Derivation records | Complete for explicit local array/view GEP paths with durable source-object identity |
| Element-path/layout records | Complete for constant in-bounds static local-array GEP paths |
| Dynamic index representative rows | Still unavailable as `missing_index_range_proof`; no loop-shape or value-name inference is allowed |
| Path/dominance validity | Not produced by idea 485; requires a separate producer for range proof and consumer-path coverage |
| Idea 484 packaging | May resume only to consume/classify the new source-object/derivation/layout carrier surface; dynamic representative packaging remains blocked until index-range/path-dominance authority exists |
| Scalar local-load production | Must remain blocked for dynamic local array loads until local-address provenance is fully available; no scalar-load consumer was enabled by idea 485 |

Representative Step 3 coverage already proves:

| Case | Expected carrier result |
| --- | --- |
| Static `[4 x i32]` local array plus constant GEP | source-object, derivation, and element-path/layout records are `available` |
| Static `[4 x i32]` local array plus dynamic GEP | source-object/derivation/path shape exists, path status is `missing_index_range_proof` |
| Local aggregate/member GEP | no local-array carrier authority is published |

Artifact:

- `build/agent_state/485_step4_residual_disposition/disposition.md`

## Suggested Next

Route the next owner to a dedicated `BIR index range proof and path-dominance
carrier` idea/packet. Its first job should be to publish explicit range proof,
dominance/path coverage, and index no-clobber facts for dynamic local array
indices before idea 484 packaging or scalar local-load production consumes
dynamic rows.

## Watchouts

- The Step 3 carrier records are producer metadata only; downstream scalar
  local-load consumption and RV64/MIR lowering remain out of scope.
- Dynamic representatives such as the `pr38048-1.c` `a#L1[i#L3][0]` shape
  still lack range/path proof and must remain unavailable until a producer
  publishes those facts.
- Aggregate/member, global, integer-pointer round-trip, variadic/runtime,
  F128, complex/vector, volatile/atomic, bootstrap, raw-shape-only, and
  target-only/final-home-only cases remain fail-closed.
- The new records are not printed yet; focused tests inspect lowered BIR
  directly.
- Idea 485 is close-ready as the source-object/derivation/layout carrier slice
  once the supervisor accepts the Step 3 code and this Step 4 disposition.

## Proof

Delegated proof:

```sh
git diff --check
```

Result: passed.
