# Step 4 Guard-Chain/Data Acceptance Review

## Scope

Reviewed the dirty Step 4 slice after `7e593eba` with focus on:

- `src/backend/mir/x86/module/module.cpp`
- `src/backend/mir/x86/module/data.cpp`
- `tests/backend/backend_x86_handoff_boundary_i32_guard_chain_test.cpp`
- `todo.md`

The lifecycle review base is `f8280a1c` (`[plan] reset step 4 loop-countdown route`), because it is the last plan checkpoint that reset the active Step 4 route for the current plan. There is 1 committed implementation commit after that base (`7e593eba`) plus the current dirty slice. The requested dirty review base is current `HEAD` at `7e593eba`.

## Findings

No blocking findings.

### Low: data-emission gate should stay bounded in the next packet

`src/backend/mir/x86/module/data.cpp:63-82` keeps named pointer emission inside defined same-module globals, excluding extern and thread-local globals. `src/backend/mir/x86/module/data.cpp:85-95` emits named pointer initializer elements only when the referenced symbol resolves to such a same-module global, so this does not open function-symbol tables, string constants, or extern pointers.

The `initializer_symbol_name` path at `src/backend/mir/x86/module/data.cpp:145-152` is intentionally narrower than a general symbol initializer: it emits only when the target global contains a named pointer element. That is acceptable for this pointer-backed aggregate slice, but future data-emitter work should replace the target-shape predicate with an explicit semantic predicate for the source initializer if this grows beyond the current mixed same-module backing aggregate case.

## Guard-Chain Identity Judgment

Acceptable. The guard-chain renderer consumes prepared identity semantically:

- Branch metadata is looked up by prepared block label and rejected when missing: `src/backend/mir/x86/module/module.cpp:5155-5160`.
- Prepared branch metadata is validated before rendering: `src/backend/mir/x86/module/module.cpp:5162-5167`.
- Branch targets come from prepared compare/control-flow labels and reject drift: `src/backend/mir/x86/module/module.cpp:5169-5175`.
- Compare operands come from `PreparedBranchCondition`, not raw compare operands: `src/backend/mir/x86/module/module.cpp:4914-4950`, with test coverage mutating the raw BIR compare while expecting unchanged output at `tests/backend/backend_x86_handoff_boundary_i32_guard_chain_test.cpp:678-735`.
- Global load/store operands are recovered through prepared memory-access records and same-module span validation: `src/backend/mir/x86/module/module.cpp:4970-5008`.
- Missing prepared global accesses are negative-tested for the pointer-backed route at `tests/backend/backend_x86_handoff_boundary_i32_guard_chain_test.cpp:1012-1076`.

I did not find raw-label fallback, consumer-synthesized branch identity, or raw global-name recovery in this dirty slice.

## Data-Emitter Judgment

Acceptable for this bounded slice. The dirty data repair emits:

- `.quad` for named pointer initializer elements that resolve to defined same-module globals.
- `.quad` for the owned pointer-backed root initializer only when its same-module backing global is the mixed pointer aggregate this packet exposes.

That is narrow enough for the current guard-chain blocker and does not accept the later multi-defined call/function-pointer/string-data family.

## Validation

`test_after.log` records the delegated command:

`cmake -S . -B build-x86 -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -DENABLE_C4C_BACKEND=ON -DC4C_ENABLE_X86_BACKEND_TESTS=ON && cmake --build build-x86 --target backend_x86_handoff_boundary_test backend_x86_prepared_handoff_label_authority_test -j2 && ctest --test-dir build-x86 -j --output-on-failure -R '^(backend_x86_handoff_boundary|backend_x86_prepared_handoff_label_authority)$'`

Configure and build passed. `backend_x86_prepared_handoff_label_authority` passed. `backend_x86_handoff_boundary` advanced through the delegated guard-chain routes and failed later at:

`bounded multi-defined-function same-module symbol-call prepared-module route: x86 prepared-module consumer did not emit the canonical asm`

That red failure is a separate later renderer boundary and is recorded in `todo.md:14-19` and `todo.md:22-24`. It does not block committing this bounded Step 4 guard-chain/data slice as progress, provided the supervisor treats this as an advancement proof rather than full Step 4 acceptance.

## Judgments

- Plan alignment: on track
- Idea alignment: matches source idea
- Technical debt: acceptable
- Validation sufficiency: narrow proof sufficient for this bounded commit; broader acceptance still needed later
- Reviewer recommendation: continue current route

