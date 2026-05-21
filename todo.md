Status: Active
Source Idea Path: ideas/open/326_aarch64_variadic_hfa_floating_residual.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Repair General Call-Boundary Preparation

# Current Packet

## Just Finished

Step 3 repaired the generalized AArch64 variadic small byval aggregate
GPR-lane publication gap for the `00204` second `stdarg` call. AST-backed
lookup was run before editing against `lower_before_call_move`,
`make_byval_register_lane_prepared_source`,
`print_aggregate_register_lane_publication_lines`, and the surrounding
call-boundary lowering path.

The root cause was precedence, not missing prepared facts. The prepared call
plan already records the `.str50` `%9s` byval arguments as size-16,
register-passed integer aggregates with contiguous destination lanes
`x2,x3`, `x4,x5`, and `x6,x7`. However, the generic
`make_prior_preserved_call_argument_source` branch ran first for preserved
stack sources and lowered each argument as a normal single stack-slot reload,
using the preserved pointer/value home size of 8 bytes. That bypassed the
existing aggregate register-lane publication path, so only `x2`, `x4`, and
`x6` were emitted.

The repair makes the prior-preserved shortcut yield for authoritative
AArch64 small register-passed byval aggregate arguments. Those moves now flow
into the existing aggregate lane path, which consumes the call-plan ABI byval
size and destination occupied-register provenance instead of guessing lanes
from slot names or synthetic stack offsets. Regenerated `.str50` code now
fills `x2/x3`, `x4/x5`, and `x6/x7`; the second `stdarg` line advances from
three `%9s` payloads to all six payloads before the later HFA output.

## Suggested Next

Classify the next `00204` residual in the later HFA output corruption after
the repaired non-HFA `%9s` byval aggregate publication. Decide whether the
remaining long-double/double/float HFA corruption is still inside idea 326's
call-boundary/HFA floating publication scope or needs a separate handoff.

## Watchouts

Do not reintroduce synthetic lane recovery by slot-name suffix or callsite
stack-offset guessing. The accepted route is precedence-based: small AArch64
register-passed byval aggregate arguments bypass the generic preserved-source
reload so the existing aggregate publication path can use prepared ABI width
and occupied lane facts.

The delegated proof command still exits nonzero because
`c_testsuite_aarch64_backend_src_00204_c` fails later in HFA output, after the
targeted `Return values:` and second `stdarg` `%9s` facts have advanced.
Treat that as a new first bad fact, not persistence of the missing aggregate
GPR-lane publication bug.

## Proof

Ran the delegated Step 3 proof:

```sh
cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(backend_.*|c_testsuite_aarch64_backend_src_00140_c|c_testsuite_aarch64_backend_src_00204_c)$' > test_after.log 2>&1
```

Result: build succeeded. The selected `backend_.*` tests passed, including
`backend_aarch64_instruction_dispatch`; `c_testsuite_aarch64_backend_src_00140_c`
passed; `c_testsuite_aarch64_backend_src_00204_c` still failed later in runtime
output. `test_after.log` is the preserved proof log and records the targeted
advancement: the second `stdarg` line now prints
`lmnopqr ABCDEFGHI ABCDEFGHI ABCDEFGHI ABCDEFGHI ABCDEFGHI` before the later
`HFA long double:` corruption.
