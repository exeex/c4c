Status: Active
Source Idea Path: ideas/open/264_phase_f4_post_f3_prepared_boundary_reassessment_gate.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Gather Post-F3 Evidence

# Current Packet

## Just Finished

Step 1, `Gather Post-F3 Evidence`, completed the required closure-note and
baseline-artifact inventory for ideas 248 through 263.

Evidence inventory:

- Route 6 call identity: idea 249 mapped scalar `i32` call argument source
  identity after Route 6/prepared `source_value_id` agreement, with public
  call-plan APIs retained as compatibility authority and riscv explicitly
  non-applicable for the selected fact family. Idea 257 confirmed the x86 path
  is already routed through
  `find_consumed_scalar_i32_call_argument_source_authority(...)`, requires
  Route 6/prepared agreement, preserves call-plan compatibility surfaces, and
  fails closed for one-sided, mismatch, unsupported, fallback, and
  policy-sensitive rows.
- Route 3 memory/source parity: idea 250 selected Route 3 `LoadLocal`
  result/source-memory identity and recorded riscv diagnostic evidence while
  leaving x86 blocked. Ideas 261 and 262 supplied the supported joined-branch
  selected-`LoadLocal` proof surface, and idea 258 closed the x86 bridge:
  x86 now requires matching Route 3 `LoadLocal` memory authority plus
  `PreparedEdgePublication::source_memory_access` before treating prepared data
  as a semantic mirror. Synthetic stale/prepared-only rows remain out of scope
  where unsupported fixtures would be required.
- Route 4/5 edge publication parity: idea 251 selected Route 5 CFG-edge
  publication source identity checked against prepared publication agreement,
  with riscv diagnostics present and x86 initially blocked. Idea 259 landed the
  x86 Route 5/prepared agreement bridge for selected dynamic `LoadLocal`
  publication moves, requiring Route 3 source-memory agreement before accepting
  agreement; non-`LoadLocal` non-agreeing rows remain on preserved prepared
  compatibility paths, and unsupported stale/synthetic fixture surfaces remain
  documented.
- Edge publication source producers: idea 252 kept producer-adapter readiness
  unsafe. X86 still lacks a direct or indirect consumer that joins prepared
  source-producer rows to the same Route 5/BIR identity with fail-closed
  rejection, and riscv has diagnostic agreement evidence only.
- Compatibility fail-closed proof: idea 254 produced the reusable
  compatibility-retention proof matrix. It is citation/test obligation only,
  not approval for implementation, demotion, deletion, public-surface
  weakening, baseline rewrites, or target-policy migration.
- Private pass context metadata: idea 255 demoted only
  `PreparedBirModule::route` behind private storage plus
  `prepared_route(const PreparedBirModule&)`. `invariants`, `completed_phases`,
  and `notes` stayed public compatibility fields with named proof gaps; they
  are not demotion-approved without a separate proof initiative.
- Liveness authority: idea 256 mapped liveness producer, direct consumer,
  compatibility, and derived target buckets. `PreparedBirModule::liveness`
  remains blocked public prepared planning authority with no safe one-reader
  implementation candidate from the completed map.
- Structural one-reader candidates: ideas 253 and 260 completed the named
  structural queue for `module`, `names`, `control_flow`, and
  `store_source_publications`. Completed packets added local agreement
  boundaries and fail-closed proof for specific readers/helpers while keeping
  public prepared aggregate fields and compatibility observable; this is not
  approval for broad `PreparedBirModule` deletion, privatization, wrapping, or
  aggregate retirement.
- Full-suite baseline repair: idea 263 classified the stale 3427/3428
  candidate as a non-reproduced, likely load-sensitive/environmental timeout,
  accepted a fresh full-suite proof, and restored the accepted baseline to
  3428/3428.
- Current baseline artifacts: `test_baseline.log` exists and records
  `100% tests passed, 0 tests failed out of 3428` for `<full-suite>`.
  `test_baseline.new.log` is absent, so no pending regression candidate is
  present.

## Suggested Next

Proceed to Step 2 by building the boundary matrix for every in-scope prepared
lookup group, prepared module field, and compatibility surface.

## Watchouts

Do not classify rows as final from this inventory alone. Compatibility strings,
helper/oracle statuses, fallback behavior, prepared printer output, wrapper
output, target output, private accessor names, and one-reader local agreement
helpers are preservation evidence or narrow agreement evidence, not broad
semantic ownership transfer proof.

## Proof

No build/test proof required for this analysis-only packet. Verified baseline
status by inspecting `test_baseline.log` and checking that
`test_baseline.new.log` is absent. No `test_after.log` was generated or
modified for this packet.
