Status: Active
Source Idea Path: ideas/open/aarch64-codegen-cpp-family-consolidation.md
Source Plan Path: plan.md
Current Step ID: Step 4
Current Step Title: Boundary Review and Broader Validation

# Current Packet

## Just Finished

Completed `plan.md` Step 4 by re-scanning the AArch64 codegen `.cpp` layout
after the Step 2 and Step 3 consolidation commits.

Final boundary rationale:

- `dispatch_publication_common.cpp` is gone; its publication glue now lives in
  `dispatch_publication.cpp`, and no live source-list references remain.
- `calls_effects.cpp` is gone; its call-printing effect helpers now live in
  `calls_printing.cpp`, and only the still-shared clobber effect API remains in
  `calls.hpp`.
- `calls_common.cpp` is intentionally retained. It is not a tiny orphan helper;
  it owns shared call ABI sizing, stack argument, register naming, variadic, and
  call-boundary utility behavior consumed across the calls family.
- `peephole.cpp` is intentionally retained as the explicit deferred peephole
  boundary described by `codegen/README.md`; folding it into an unrelated owner
  would hide the current no-live-pass decision.
- `compatibility_projection.cpp` is intentionally retained. It owns the
  compatibility projection between lowered machine functions and legacy
  publication records, which remains a named route concept rather than local
  glue for `module_compile.cpp`.
- `module_compile.cpp` is intentionally retained as the internal prepared-module
  compile coordinator and public build/compile handoff surface.
- `dispatch_diagnostics.cpp` is intentionally retained as the diagnostics
  ownership surface for dispatch and call-lowering failures.

The remaining small files therefore still correspond to durable implementation
concepts or explicitly deferred route surfaces. The completed merges moved only
AArch64-local publication and calls helpers within their existing families; no
target-independent logic was moved into AArch64.

## Suggested Next

No further implementation packet is needed for this runbook. The source idea
acceptance criteria appear satisfied: a small documented set of `.cpp` files
was merged, remaining files have durable boundary names, and fresh backend
proof is recorded below.

Any future consolidation beyond this scope, such as reconsidering the broader
calls-family layout or changing the deferred peephole route, should start as a
separate idea.

## Watchouts

- Do not fold the retained tiny route surfaces into larger files without a new
  source idea; they are intentionally retained under this runbook.
- Lifecycle close/deactivate belongs to the supervisor and plan owner, not this
  executor packet.

## Proof

Proof command run and passed:

```sh
{ cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_'; } 2>&1 | tee test_after.log
```

Result: build passed; `^backend_` CTest subset passed 149/149 tests.

Proof log: `test_after.log`.
