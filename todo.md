# Current Packet

Status: Active
Source Idea Path: ideas/open/716_prealloc_current_block_routing_authority_closure.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Prove owner attachment and lifetime independently

## Just Finished

- Step 2 proved the independent owner-attachment seam at the production
  `make_function_lowering_context` boundary: it constructs and attaches the
  `PreparedFunctionLookups` shared owner and its non-owning context view.
- The registered `backend_prealloc_current_block_lookup_attachment_lifetime`
  contract now proves a copied `FunctionLoweringContext` retains the shared
  lookup lifetime after the original owner context leaves scope, while a
  context with no attachment remains null and fails closed.
- No Route 5 behavior, routing authority, or positive incoming-expression
  answer is asserted by this packet.

## Suggested Next

- Execute Step 3 by proving complete prepared incoming-expression authority is
  independent of Route 5 diagnostic identity, holding the authoritative edge
  facts constant while Route 5 diagnostics vary.

## Watchouts

- Step 2 establishes attachment and lifetime only; it deliberately grants no
  routing authority and cannot substitute for Step 3's complete prepared edge
  fact proof.
- Ideas 713 and 705 remain open and blocked pending handback.

## Proof

- Passed `cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_' 2>&1 | tee test_after.log`.
- Result: 316/316 backend tests passed; the supervisor-selected proof was
  sufficient; proof log: `test_after.log`.
