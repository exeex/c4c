Status: Active
Source Idea Path: ideas/open/109_hir_to_lir_struct_layout_lookup_by_struct_name_id.md
Source Plan Path: plan.md
Current Step ID: Step 7
Current Step Title: Validate Parity And Prepare For The Printer Audit

# Current Packet

## Just Finished

Completed `plan.md` Step 7 parity validation plumbing and broad proof.
`lookup_structured_layout` now records module-level
`LirStructuredLayoutObservation` entries for structured lookup attempts, with
site labels for field chains, indexed GEPs, constant initializers, object
alignment, variadic aggregate args, and `va_arg` aggregates. The verifier now
checks those observations and reports any checked structured-vs-legacy layout
mismatch with the lookup site, type, legacy size/alignment, legacy field types,
and structured field types. No LLVM emission authority or legacy fallback
behavior changed.

## Suggested Next

Supervisor can decide whether Step 7 is acceptance-ready for plan-owner review
or whether idea 110 printer-audit readiness needs an additional independent
review packet.

## Watchouts

Structured layout observations are inert debug/verifier state; lowering still
uses `layout.legacy_decl` for size, alignment, ABI, and emitted LLVM decisions.
The full suite verified that no checked structured-vs-legacy layout mismatches
were present under current coverage.

## Proof

Delegated proof passed and wrote `test_after.log`:
`{ cmake --build --preset default && ctest --test-dir build -j --output-on-failure; } > test_after.log 2>&1`.

The full suite passed: 2980/2980 tests.
