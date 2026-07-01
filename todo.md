Status: Active
Source Idea Path: ideas/open/490_dynamic_local_array_lir_producer_path_no_clobber_certificate.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Implement Or Route Certificate Producer

# Current Packet

## Just Finished

Completed Step 3 as a route-only packet for idea 490.

Targeted inspection compared current prepared/BIR surfaces against the Step 2
contract and found no safe implementation path.

| Required producer | Step 3 finding |
| --- | --- |
| LIR producer key | Present on `LocalArrayElementPathRecord::lir_producer_*`. |
| Prepared branch/compare proof source | Candidate `PreparedBranchCondition` fused compare facts and operand producer helpers exist. |
| Selected proof edge/path/order certificate | Missing durable producer keyed to `lir_producer_lookup_key`, selected edge/outcome, path coverage, dominance/guard validity, and truthful same-block ordering. |
| Dynamic-index interval effects | Missing redefinition, phi/alias, call/helper, inline asm, publication/move-bundle, and parallel-copy effect classifier over the selected path. |
| Step 3 decision | No implementation selected. First lower owner: `dynamic local-array selected proof-edge path certificate`. |

Supporting artifact:

- `build/agent_state/490_step3_certificate_producer_route/route.md`

## Suggested Next

Execute Step 4 residual disposition for idea 490, recording close/split readiness
around the first lower owner `dynamic local-array selected proof-edge path
certificate` and preserving the later interval-effects owner boundary.

## Watchouts

- Do not populate idea 486 checker inputs directly in idea 490.
- Do not resume idea 489 proof population until a selected proof-edge path
  certificate and dynamic-index interval effects exist.
- Keep `lir_producer_instruction_index` as a LIR producer-site coordinate; do
  not reinterpret it as a prepared traversal or BIR `Block::insts` coordinate.
- Same-block rows need a truthful ordering proof or must remain unavailable.
- Keep idea 484 packaging, scalar local-load consumption, and RV64/MIR lowering
  out of scope.
- Existing untracked `review/*.md` files were not touched.

## Proof

Step 3 route-only validation:

```sh
git diff --check
```

Result: passed.
