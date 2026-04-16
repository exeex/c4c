Status: Active
Source Idea: ideas/open/12_prealloc_cpp_rs_convergence_and_activation.md
Source Plan: plan.md

# Current Packet

## Just Finished

Started Step 2 by making the active C++ stack-layout path elide frame slots for
dead local-slot objects, updating the stack-layout notes to report home-slot
emission, and adding `backend_prepare_stack_layout` to prove prepared output
changes for a real function shape inside c4c.

## Suggested Next

Continue Step 2 in `stack_layout`: compare the C++ alloca/copy-coalescing and
slot-assignment helpers to the retained Rust references, then activate the next
bounded semantic gain beyond dead-slot elision instead of leaving the remaining
coalescing/tiering hints metadata-only.

## Watchouts

- the active C++ route now keeps dead local-slot objects in prepared metadata
  but only assigns frame slots to objects that still require home storage
- `analysis.cpp` plus `regalloc_helpers.cpp` still keep live local-slot objects
  conservative, so richer Rust-like tiering is not active yet
- keep `.rs` files as references until the final comparison pass is complete
- acceptance requires both `.cpp` vs `.rs` comparison and runtime proof in c4c
- do not let `liveness` or `regalloc` fall back to object identity for value
  tracking

## Proof

Ran `cmake --build --preset default -j4 && ctest --test-dir build -j
--output-on-failure -R '^backend_'` successfully; canonical proof log:
`test_after.log`.
