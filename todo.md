Status: Active
Source Idea Path: ideas/open/166_hir_rendered_registry_mirror_retirement_audit.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Process Remaining Module and Compile-Time Registries

# Current Packet

## Just Finished

Completed plan Step 4 for `CompileTimeState` value-binding registries:
global enum-constant and global const-int lookups now have structured helper
entry points that prefer complete value-binding keys and fail closed after
complete structured misses instead of reopening rendered name maps. Explicit
no-metadata/rendered compatibility calls still use the rendered maps.

Extended focused frontend HIR coverage proving stale rendered enum/const-int
maps are not used after complete structured misses while no-metadata calls
retain compatibility behavior.

## Suggested Next

Continue Step 4 by auditing the remaining module-level compile-time registry
handoffs for any direct raw-map consumers that should move behind the fenced
structured helper entry points.

## Watchouts

The new `CompileTimeState` helpers fence value-binding lookups, but existing
consteval environment construction still passes raw rendered and structured maps
because `ConstEvalEnv` already performs per-domain miss fencing. Keep lowerer
route-local maps, ctor/dtor maps, and overload maps out of this packet unless a
future Step 4 handoff explicitly owns them.

## Proof

Ran the delegated proof command:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^frontend_hir_tests$' > test_after.log 2>&1`.

Result: passed. `test_after.log` contains the focused `frontend_hir_tests`
ctest pass.
