# Current Packet

Status: Active
Source Idea Path: ideas/open/748_lir_memcpy_selected_pointer_object_authority_publication.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Prove the producer and publish the consumer handoff

## Just Finished

- Plan Step 2 complete: reachable LIR verification now binds the one selected
  `LirMemcpyOp::selected_authority` to its current-function pointer authority.
  It rejects missing/duplicate selected rows and malformed pointer identities,
  object links/owners, i64-positive size, and live-site facts without reading
  display operands. Focused coverage exercises each typed rejection boundary.

## Suggested Next

- Execute Step 3 only: establish the producer/verifier proof and write the
  durable selected-row authority handoff for idea 734 at return point Step
  7.20, without receiver implementation.

## Watchouts

- Verification recognizes selected status solely through the structured
  descriptor and current-function selected pointer carrier. Unselected memcpy
  rows retain prior verification behavior; no display operand text is parsed.

## Proof

- `cmake --build --preset default && ctest --test-dir build -j
  --output-on-failure -R '^backend_' | tee test_after.log` passed: 5/5 backend
  tests, including `backend_lir_selected_pointer_authority`. The
  supervisor-selected subset was sufficient for this verifier packet;
  `test_after.log` is the proof log.
