# Current Packet

Status: Active
Source Idea Path: ideas/open/705_prepared_fact_boundary_from_bir_views.md
Source Plan Path: plan.md
Current Step ID: 2.3b.3
Current Step Title: Retire the bounded Route 5 compatibility consumer

## Just Finished

- None; lifecycle activation reset after closing idea 713.

## Suggested Next

- Execute Step 2.3b.3: remove the Route 5 public compatibility payload and
  make the bounded AArch64 consumer read only the accepted prealloc-owned
  stable-key routing result.

## Watchouts

- Preserve the accepted owner-attached query and fail-closed ambiguity
  contract; do not reconstruct named evidence, publications, transfers, or
  selection authority in AArch64.
- Keep common MIR queries, unrelated producer families, x86/RV64
  materializers, and target-owned publication authority out of scope.

## Proof

- Pending Step 2.3b.3 implementation proof. The accepted consumer baseline is
  commit `1394423de`, final review is
  `review/idea713_step4_bounded_consumption_final_review.md`, and the
  preserved broader backend proof reports 329/329.
