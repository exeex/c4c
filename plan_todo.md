# std::vector Bring-up Todo

Status: Active
Source Idea: ideas/open/std_vector_bringup_plan.md
Source Plan: plan.md

## Active Item

- Step 2 / Step 3: continue reducing the next `std::vector` parser blocker now
  that explicit `obj.operator->()` member calls are handled correctly

## Completed

- activated the open `std::vector` bring-up idea into `plan.md`
- reproduced the current `std_vector_simple.cpp` failure and narrowed the first
  actionable parse blocker to `bits/ptr_traits.h:215` on `__ptr.operator->()`
- added `operator_arrow_explicit_member_call_parse.cpp` and taught postfix
  parsing to accept explicit operator-name member access after `.` / `->`
- verified the reduced parse test, `bits/ptr_traits.h`, and the full suite
  after the fix; suite moved from `2248/2249` passing with one unrelated fail
  to `2250/2250` passing after the new regression coverage landed

## Next Intended Slice

- reduce the next `bits/stl_iterator.h` parser failure, starting from
  `__miter_base(reverse_iterator<_Iterator> __it)` at line 672 and the related
  constructor signatures at lines 704/805/920, into the smallest standalone
  parser test before changing the parser again

## Blockers

- none recorded yet

## Resume Notes

- `plan.md` did not exist at iteration start; the source idea was activated from
  `ideas/open/std_vector_bringup_plan.md`
- current documented repro command: `./build/c4cll --parse-only tests/cpp/std/std_vector_simple.cpp`
- compare-mode validation is deferred until the case reaches codegen
- isolated header repro: `./build/c4cll --parse-only /tmp/include_ptr_traits.cpp`
  fails at `bits/ptr_traits.h:215` with `expected RPAREN but got 'operator'`
- post-fix `./build/c4cll --parse-only /tmp/include_ptr_traits.cpp` succeeds
- current `std_vector_simple.cpp` repro has advanced past `ptr_traits.h`; the
  first remaining parse errors are in `bits/stl_iterator.h`,
  `bits/predefined_ops.h`, and later headers
