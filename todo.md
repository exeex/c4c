# Semantic Call And Runtime Boundary From BIR

Status: Active
Source Idea: ideas/open/47_semantic_call_runtime_boundary.md
Source Plan: plan.md
Current Plan Focus: ordered step 3, lower runtime and intrinsic families semantically

# Current Packet

## Just Finished
- fixed GNU statement-asm parsing so the empty-template fold only erases non-volatile, clobber-free cases whose outputs are fully semantics-preserving rewrites; volatile or untied-output forms now survive as `NK_ASM`
- added the scalar `i32` backend route proof for `__asm__ volatile("" : "=r"(y) : "r"(x));`, and the semantic BIR now preserves a typed `bir.call i32 llvm.inline_asm(...)` whose result is stored to `y` and returned

## Suggested Next
- widen semantic runtime-family coverage from this parser fix to nearby inline-asm shapes that still depend on the same preservation rule, especially multi-output or tied-output cases
- if inline-asm route work pauses here, move back to another ordered step 3 runtime/intrinsic family with a similarly direct semantic-BIR proof slice

## Watchouts
- the empty-template parser fold is still intentionally active for non-volatile, clobber-free outputs that are all `+` read-write or digit-tied copies; changing that route further should keep the “all outputs must be representable” guard intact
- this packet only proves the single-result scalar route on x86_64 semantic BIR; it does not establish coverage for multi-result GNU asm or target-specific codegen details

## Proof
- `bash -lc 'cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R "^backend_"' > test_after.log 2>&1`
- passed after the parser preservation fix and backend proof-case addition; the new route test now observes `bir.call i32 llvm.inline_asm(...)` in semantic BIR for the scalar result path
- proof log preserved at `test_after.log`
