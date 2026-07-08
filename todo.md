Status: Active
Source Idea Path: ideas/open/605_bir_local_memory_alloca_and_scalar_semantics.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Select Alloca And Scalar-Local Proof Rows

# Current Packet

## Just Finished

Activated Step 1 from `plan.md`: select representative alloca and scalar/local-memory mixed proof rows before implementation begins.

## Suggested Next

Refresh or inspect the RV64 gcc_torture failure artifacts for alloca local-memory and scalar/local-memory mixed semantic stops, then record the selected proof rows, guard rows, starting expectations, and delegated proof command here.

## Watchouts

- Keep load, store, and GEP producer failures separate from this alloca/scalar-local route.
- Do not pull prepared/RV64 consumer, global initializer, ABI stack-frame, runtime, expectation, unsupported-marker, allowlist, timeout, or accounting work into this plan.
- Reject named-case fixes, especially routes centered only on `src/20180921-1.c`.

## Proof

No validation run for activation; lifecycle-only plan/todo creation.
