Status: Active
Source Idea Path: ideas/open/19_x86_riscv_prepared_edge_publication_handoff.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Final Handoff and Validation

# Current Packet

## Just Finished

Completed Step 5 of `plan.md`: wrote the final handoff notes for idea 19 using
the accepted Step 4 proof as the committed validation basis.

The active source idea's acceptance criteria are now evaluable from this
handoff:

- Shared prepared edge-publication records and helpers were audited from the
  x86/RISC-V consumer perspective, and no required shared contract repair was
  identified.
- AArch64-only emission details remain outside shared prepare; the shared facts
  carry semantic edge-flow authority rather than register names, scratch policy,
  clobber sequencing, move instruction spelling, stack operand syntax, or
  assembly formatting.
- x86 has a clear minimal handoff path through the existing shared prepared
  lookup authority:
  `x86::ConsumedPlans::shared_function_lookups()->edge_publications`.
- RISC-V has the same intended shared authority, but its first consumer packet
  should wait until it has a prepared-module lowerer entry point and prepared
  function/block traversal surface.
- The focused proof is the Step 4 x86 consumer test: a non-AArch64 consumer
  reads indexed shared edge-publication facts and verifies only semantic
  predecessor/successor edge identity, source/destination prepared value ids,
  home/storage kinds, block-entry phase, execution site, step ordering, and
  links back to the shared prepared source records.

Recommended next initiative: x86 consumer implementation. x86 already has a
prepared decoded-home fixture and the lookup surface needed for the smallest
real consumer slice. RISC-V consumer implementation should follow after its
prepared-module lowerer entry point and prepared function/block traversal are
available. No shared prepared contract repair is recommended from this handoff.

## Suggested Next

Supervisor should review and close or retire the active plan for idea 19, then
open the next source initiative for an x86 prepared edge-publication consumer
implementation.

## Watchouts

- The next x86 implementation should consume indexed shared
  edge-publication authority; it should not add a second x86-specific
  edge-copy authority or broaden wrappers without a real consumer need.
- Target-local responsibilities remain outside shared prepare for both x86 and
  RISC-V: scratch selection, clobber avoidance, physical register spelling,
  register-class constraints, stack operand syntax, move instruction spelling,
  branch/control-flow emission, and final assembly formatting.
- RISC-V remains a readiness follow-up, not the immediate consumer target,
  because it still needs a prepared-module lowerer entry point and prepared
  function/block traversal before an edge-publication consumer can be attached
  cleanly.
- Future consumer proofs should continue to reject testcase-shaped matching and
  expectation downgrades; progress should come from semantic lowering against
  shared prepared facts.

## Proof

No new build was required for this documentation-only Step 5 packet. This
handoff cites the existing accepted Step 4 proof:

`cmake --build --preset default > test_after.log 2>&1 && ctest --test-dir build -j --output-on-failure -R '^(backend_x86_|backend_prepared_lookup|backend_prepare_liveness|backend_prealloc_block_entry_publications)' >> test_after.log 2>&1`

Result: passed 9/9. The supervisor regression guard accepted the result with a
non-decreasing pass count and no new failures. The accepted proof log was rolled
forward to canonical `test_before.log` after the Step 4 commit.
