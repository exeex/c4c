Status: Active
Source Idea: ideas/open/12_prealloc_cpp_rs_convergence_and_activation.md
Source Plan: plan.md

# Current Packet

## Just Finished

Activated the prealloc C++/Rust convergence idea into the active runbook and
captured the phase-contract, regalloc-activation, and acceptance-scaffolding
constraints as explicit plan rules.

## Suggested Next

Start Step 2 on `stack_layout`: compare each active `stack_layout/*.cpp` file to
its retained Rust reference and close the biggest semantic gaps that still leave
the C++ route provisional.

## Watchouts

- do not let `liveness` or `regalloc` fall back to object identity for value
  tracking
- keep `.rs` files as references until the final comparison pass is complete
- acceptance requires both `.cpp` vs `.rs` comparison and runtime proof in c4c
- `regalloc.cpp` must become active before this initiative can be considered
  autonomy-ready

## Proof

Lifecycle-only activation; no code build or test was required for this packet.
