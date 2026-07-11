# Current Packet

Status: Active
Source Idea Path: ideas/open/718_transitive_current_block_incoming_expression_authority_decomposition.md
Source Plan Path: plan.md
Current Step ID: 8
Current Step Title: Implement no-policy and zero-owner-fact semantics

## Just Finished

- Completed plan Step 8 by making the policy query result itself authoritative
  only when its state is `AuthoritativeFacts` and its routing consumption is
  available.
- Strengthened the focused state matrix for absent policy, absent owner,
  attached zero facts, attached non-applicable facts, and authoritative facts;
  owner presence alone remains non-authoritative and fail-closed.

## Suggested Next

- Execute plan Step 9 as the bounded stable-key owner-fact composition packet.

## Watchouts

- The zero-fact and non-applicable-fact states deliberately share
  `AttachedOwnerWithoutApplicableFacts`; neither can be consumed as authority.
- The backend subset still has only the known Step 9 stable-key composition
  focused gap; no unrelated regression was introduced by this packet.

## Proof

- Ran `cmake --build --preset default && ctest --test-dir build -j
  --output-on-failure -R '^backend_' > test_after.log 2>&1` exactly as delegated.
- Build passed; 323/324 backend tests passed. The no-policy/zero-owner-fact
  probe is green. Only the known stable-key composition focused probe remains
  red. Proof log: `test_after.log`.
