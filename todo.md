Status: Active
Source Idea Path: ideas/open/10_bir-global-initializer-family-extraction.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Map Initializer Ownership and Move Boundary

# Current Packet

## Just Finished

Activation created the runbook and executor scratchpad for Step 1.

## Suggested Next

Execute Step 1 from `plan.md`: inspect `globals.cpp`, `lowering.hpp`, and the
relevant helper call graph, then record the exact move set and stay set before
editing implementation files.

## Watchouts

- Preserve behavior; this extraction is mechanical.
- Do not add new headers.
- Keep `lower_minimal_global`, `lower_string_constant_global`, and
  `resolve_known_global_address` in `globals.cpp` unless the implementation
  proves a tiny wrapper is unavoidable.
- Do not rewrite test expectations.

## Proof

Lifecycle activation only; no build or test proof required yet.
