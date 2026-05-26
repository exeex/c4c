Status: Active
Source Idea Path: ideas/open/29_riscv_prepared_edge_publication_pointer_base_policy_broadening.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Implement or Preserve Fail-Closed Pointer-Base Policy

# Current Packet

## Just Finished

Step 2 implemented the selected RISC-V prepared edge-publication
`PointerBasePlusOffset -> Register` large-delta policy for register bases.
Distinct destination registers now render large deltas as `li dst, delta`
followed by `add dst, base, dst`, while zero and signed-12-bit deltas keep the
existing `mv`/`addi` rendering.

Focused coverage now includes a positive large-delta distinct-destination case
and a fail-closed destination/base alias case. Existing fail-closed coverage for
missing shared lookup/publication, non-move publications, malformed source or
destination homes, missing/unresolved pointer bases, absent deltas,
non-register base homes, and pointer-base stack destinations remains in place.

## Suggested Next

Run the route-quality review for the Step 2 slice before broadening additional
pointer-base prepared edge-publication forms.

## Watchouts

The implementation intentionally does not add scratch-register semantics. Large
deltas where the destination aliases the base register remain unsupported until
a separate scratch policy is delegated.

Keep shared `edge_publications` and prepared value-home lookup as the only
semantic authorities; this slice did not add predecessor/successor scanning or
RISC-V-local edge-publication discovery.

## Proof

Passed: `cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_riscv_prepared_edge_publication$' > test_after.log 2>&1`

Proof log: `test_after.log`.
