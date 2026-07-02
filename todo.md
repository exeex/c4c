Status: Active
Source Idea Path: ideas/open/520_bir_render_owner_preservation.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Apply Minimal Ownership Change Or Preserve Decision

# Current Packet

## Just Finished

Step 3 - Apply Minimal Ownership Change Or Preserve Decision is complete for
idea 520.

Moved the definitions of `render_type`, `render_binary_opcode`, and
`render_cast_opcode` out of `src/backend/bir/bir.cpp` and into the new narrow
render translation unit `src/backend/bir/bir_render.cpp`. Public declarations
remain in `src/backend/bir/bir.hpp`; no printer, route, validation, test,
expectation, or output spelling changes were made.

Fixed the narrow direct-source backend BIR test metadata gap by adding
`src/backend/bir/bir_render.cpp` to `backend_lir_to_bir_notes_test`, the target
that compiles `bir.cpp` and LIR-to-BIR sources directly without linking through
the backend source set.

## Suggested Next

Supervisor can review the completed Step 3 slice, decide whether additional
acceptance validation is needed, and commit the coherent code plus `todo.md`
update if accepted.

## Watchouts

`src/backend/CMakeLists.txt` picked up `bir_render.cpp` through
`GLOB_RECURSE CONFIGURE_DEPENDS`. `backend_prepare_phi_materialize_test` also
names `bir.cpp` directly, but it links `c4c_backend`, which supplies the new
render TU; the delegated backend proof passed without adding a duplicate source
entry there. `bir_printer.cpp` remains rejected for this route because the
audited consumer map is mixed, not printer-only.

## Proof

Delegated proof command:

```sh
git diff --check && { cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_'; } > test_after.log 2>&1
```

`git diff --check` passed. `cmake --build --preset default` passed, and
`ctest --test-dir build -j --output-on-failure -R '^backend_'` passed.
`test_after.log` contains the full proof log.
