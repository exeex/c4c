Status: Active
Source Idea Path: ideas/open/401_rv64_object_route_scalar_and_floating_edge_lowering.md
Source Plan Path: plan.md
Current Step ID: Step 1
Current Step Title: Classify Scalar And Floating Edge Shapes

# Current Packet

## Just Finished

Lifecycle switched from the closed `I16` formal ABI publication repair to
active idea 401. No implementation packet has run for this plan yet.

## Suggested Next

Execute Step 1 by classifying the prepared forms behind
`unsupported_scalar_compare_trunc` and `unsupported_floating_cast`, including
the newly exposed `src/20000403-1.c` scalar compare/trunc blocker.

## Watchouts

- `ideas/closed/403_prepared_i16_formal_abi_publication.md` is complete; do not
  reopen `I16` formal ABI publication in this object-route plan.
- Do not special-case `src/20000313-1.c`, `src/20020225-2.c`,
  `src/20000403-1.c`, function names, or constants.
- Split missing prepared producer facts into a separate idea instead of
  compensating inside RV64 object emission.

## Proof

Lifecycle-only switch. Close gate for idea 403 passed with canonical backend
regression logs in non-decreasing mode: 326 passed before, 326 passed after, no
new failing tests.
