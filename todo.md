Status: Active
Source Idea Path: ideas/open/54_aarch64_global_value_materialization_consumer_authority_repair.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Consolidate Shared Query Shape

# Current Packet

## Just Finished

Completed Step 4 audit for the shared prepared global-load query surface.

`prepare::find_prepared_global_load_access(...)` and
`prepare::find_prepared_same_block_global_load_access(...)` return the same
`PreparedSameBlockGlobalLoadAccess` record and accept only a prepared memory
access whose result name matches the `LoadGlobal` result, whose address is a
global symbol with a symbol name, and whose address supports base-plus-offset
emission.

The ordinary current-instruction globals consumer uses the direct
instruction-index query, while both non-current value materialization consumers
(`fp_value_materialization.cpp` and `dispatch_value_materialization.cpp`) use
the same same-block scalar-producer query before deriving link name,
direct/GOT policy, and byte offset from the prepared access. Dispatch
materialization remains mechanically compatible with the shared shape: it
receives the same load/access pair and applies only local register/mnemonic
selection around the prepared authority.

## Suggested Next

Delegate Step 5 final validation for the global value materialization authority
route.

## Watchouts

No code compatibility fix was required. The shared query still encodes the
global-symbol/base-plus-offset preconditions that dispatch assumes before
dereferencing the prepared symbol name.

## Proof

No build or test run for this audit-only Step 4 packet. No code changed, and
the delegated proof contract said not to modify logs when no code changes were
made.
