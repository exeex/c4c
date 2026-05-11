Status: Active
Source Idea Path: ideas/open/163_backend_semantic_bir_route_alignment.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Align Semantically Correct Stale BIR Snippets

# Current Packet

## Just Finished

Step 3 aligned the stale semantic BIR snippets for tests `134` and `136`: `backend_codegen_route_x86_64_inline_asm_output_readwrite_i32_observe_semantic_bir` and `backend_codegen_route_x86_64_inline_asm_output_readwrite_ptr_observe_semantic_bir`. The updates are semantic-only: regenerated BIR now spells readwrite inline asm constraints as the tied output/input pair `constraints="=r,0"` instead of the frontend shorthand `+r`, while preserving the empty asm text, sideeffect marker, one input operand, call result type, store-back behavior, and return semantics for both the `i32` and `ptr` cases.

The Step 1 31-failure inventory remains available in committed `todo.md` at `86ba9f59b` (`[todo_only] Inventory backend semantic BIR route failures`); this packet keeps that inventory referenced rather than duplicating it over the current execution state.

## Suggested Next

Return to the supervisor for commit-boundary selection for this two-test inline asm readwrite semantic snippet alignment, or delegate the next narrow family from the committed Step 1 inventory if this slice is accepted.

## Watchouts

- The snippet update preserves the same required semantic operations and forbidden guards; it does not mark the route unsupported, remove forbidden snippets, or broaden matching.
- This packet touches only the delegated inline asm readwrite `i32` and `ptr` semantic BIR snippets and leaves unrelated route snippets unchanged.

## Proof

Ran the delegated proof into `test_after.log`; build succeeded and both route tests passed:

`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R 'backend_codegen_route_x86_64_inline_asm_output_readwrite_(i32|ptr)_observe_semantic_bir$'`
