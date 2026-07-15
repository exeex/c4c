# Current Packet

Status: Active
Source Idea Path: ideas/open/795_lir_body_parameter_authority_handoff.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Publish the selected parameter-index authority
你該做code review了

## Just Finished

795 Step 1 traced `pr21173.c` from the GEP verifier failure to `foo` parameter
`p#P0`: it is a one-to-one native `char*` body parameter, not an ABI-expanded
form. `init_fn_ctx` publishes only `%p.p` spelling and the ordinary parameter
rvalue consumer returns it as raw text; the pointer-difference RHS consequently
reaches the direct pointer-compound GEP without integer/SSA authority. Durable
route evidence: `review/795_step1_parameter_index_trace.md`.

## Suggested Next

795 Step 2: publish checked current-function authority for this native scalar
parameter surface at the `init_fn_ctx`/parameter-DeclRef handoff, then keep
proof bounded to its pointer-to-integer-derived GEP-index chain.

## Watchouts

Do not use rendered parameter identity, weaken GEP verification, or absorb the
`20060910-1.c` PHI producer failure owned by 806. `p#P0` is native pointer, not
native integer; any following raw cast/bin authority loss is only the selected
pointer-to-integer index chain, not authorization for broad ABI work.

## Proof

Trace-only packet: `./build/c4cll --codegen llvm
tests/c/external/gcc_torture/src/pr21173.c` failed as expected with
`LirGepOp.indices.value: authoritative GEP index requires integer or SSA
authority`; `--dump-hir` identified `foo(p: char*)` and `p#P0` as the selected
source. No build/CTest baseline was requested or run; no `test_after.log` was
created for this trace-only packet.
