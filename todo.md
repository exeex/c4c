Status: Active
Source Idea: ideas/open/54_x86_backend_c_testsuite_capability_families.md
Source Plan: plan.md

# Current Packet

## Just Finished

Executed `plan.md` Step 1 by re-running the full `x86_backend` checkpoint and
re-baselining the current local-memory frontier. The next honest candidate is
one bounded addressed-local-scalar load/store lane: `00078` fails in the load
local-memory family, while `00163` and `00171` fail in the store
local-memory family. Simpler address-taking probes such as `00073` and `00103`
already pass, so the route can stay narrower than general pointer arithmetic
or indirect prepared-module work.

## Suggested Next

Execute `plan.md` Step 2 by naming the bounded local-memory lane explicitly as
addressed local scalar load/store through direct `&local` capture and
dereference, with `00078`, `00163`, and `00171` as the proving cluster.
Keep `00207` alloca/VLA plus goto control flow, `00176` / `00217`-style
indexed `gep` traffic, and the prepared-module `00189` indirect/global-
function-pointer boundary out of scope for that packet.

## Watchouts

- Keep the admitted `00131` / `00211` / `00210` prepared-module lane as
  regression baseline coverage.
- Keep `00189` explicit as the adjacent indirect/global-function-pointer plus
  indirect variadic-runtime boundary rather than silently widening into it.
- Keep `00207` out of the next packet; its VLA/alloca plus goto shape is not
  the same bounded lane as direct addressed local scalars.
- Keep `00176` and `00217` out of the next packet; they widen into indexed
  `gep` traffic rather than direct addressed local scalar dereferences.
- Keep `00057` and `00124` out of the next packet; they remain emitter and
  scalar-control-flow neighbors rather than the named local-memory cluster.
- Do not weaken `x86_backend` expectations or add testcase-shaped recognizers.

## Proof

Step 1 proof:
`cmake --build --preset default && ctest --test-dir build -L x86_backend --output-on-failure > test_after.log 2>&1`

Result: the full `x86_backend` checkpoint reports `70/220` passing and
`150/220` failing. For the local-memory route, `00078` fails in the load
local-memory family, `00163` and `00171` fail in the store local-memory
family, and `00207` remains a larger alloca/VLA neighbor. The admitted
prepared-module baseline lane (`00131`, `00211`, `00210`) remains separately
tracked, and the adjacent `00189` prepared-module indirect/global-function-
pointer boundary is still unsupported. Proof log path: `test_after.log`.
