Status: Active
Source Idea Path: ideas/open/541_rv64_object_call_variadic_return_fragment_cleanup.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Map Call-Family Ownership

# Current Packet

## Just Finished

Lifecycle activation created `plan.md` and this executor-compatible `todo.md` for Step 1.

## Suggested Next

Execute Step 1 from `plan.md`: map the call, variadic, prologue/epilogue, and return helper groups; name the first safe extraction target; record parked helpers and the exact proof command for Step 2.

## Watchouts

- Keep this run behavior-preserving.
- Do not change variadic admission, call-boundary effects, preserved-register behavior, sret/byval behavior, before-return semantics, diagnostics, runtime expectations, unsupported markers, or object bytes.
- Treat legacy `calls.cpp`, `variadic.cpp`, `prologue.cpp`, and `returns.cpp` as references unless a later slice creates live build ownership.
- Do not move final object module assembly, public ELF entrypoints, or `prepared_function_to_object_function`.

## Proof

Activation is lifecycle-only; no build or test proof was run.
