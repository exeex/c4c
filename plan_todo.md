# std::vector Bring-up Todo

Status: Active
Source Idea: ideas/open/std_vector_bringup_plan.md
Source Plan: plan.md

## Active Item

- Step 2 / Step 3: reduce the next post-`__miter_base` `std::vector` blocker,
  starting with dependent template-parameter type handling in
  `bits/stl_iterator.h` / `bits/predefined_ops.h`

## Completed

- activated the open `std::vector` bring-up idea into `plan.md`
- reproduced the current `std_vector_simple.cpp` failure and narrowed the first
  actionable parse blocker to `bits/ptr_traits.h:215` on `__ptr.operator->()`
- added `operator_arrow_explicit_member_call_parse.cpp` and taught postfix
  parsing to accept explicit operator-name member access after `.` / `->`
- verified the reduced parse test, `bits/ptr_traits.h`, and the full suite
  after the fix; suite moved from `2248/2249` passing with one unrelated fail
  to `2250/2250` passing after the new regression coverage landed
- added `template_unresolved_param_type_parse.cpp` and taught declaration
  parsing to admit unresolved `Identifier<...>` parameter-type starts while
  keeping expression parsing from treating plain relational expressions as
  template-id casts
- verified the new reduced parse test, nearby operator parse coverage, and the
  full suite; suite moved from `2250/2250` to `2251/2251`

## Next Intended Slice

- reduce `bits/stl_iterator.h:1494` (`typedef _Iterator pointer;`) and the
  first remaining `bits/predefined_ops.h:150` constructor-parameter failure
  into the smallest standalone parser/frontend repro before changing more
  dependent-type handling

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
- this iteration starts by reducing the first `stl_iterator.h` failure near
  `__miter_base(reverse_iterator<_Iterator> __it)` before touching later
  headers
- current `std_vector_simple.cpp` repro no longer reports the old
  `__miter_base(reverse_iterator<_Iterator> __it)` / `move_iterator<_Iterator>
  __it` `expected RPAREN but got '<'` failures
- the new reduced regression is
  `tests/cpp/internal/postive_case/template_unresolved_param_type_parse.cpp`
- next visible blockers start earlier in the header stack at
  `bits/stl_iterator.h:1494`, then `bits/predefined_ops.h:150`
