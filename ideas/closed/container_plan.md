# Container Usability Plan (Milestone 3)

## Goal

Validate that the landed operator overload, iterator, and range-for support
is sufficient to compile and run a small fixed-storage vector-like container
with the API surface defined in `plan.md` Milestone 3.

## Parent Plan

`plan.md` — Milestone 3: Small vector-like usability

## Scope

### In scope

- fixed-storage container with element array + length
- `size()`, `empty()`, `data()`
- `front()`, `back()`
- `operator[]` with const/non-const overloads
- `push_back()`, `pop_back()`
- `begin()` / `end()` returning iterator objects
- manual iterator loop traversal
- `range-for` traversal with `auto`
- integrated smoke test exercising all APIs together

### Out of scope

- dynamic allocation / heap storage
- move semantics
- copy constructor / assignment operator
- exception safety
- template containers (use concrete `int` element type)

## Design

The container is a simple `FixedVec` with:

```cpp
struct FixedVec {
    int buf[16];
    int len;

    int size() const;
    bool empty() const;
    int* data();

    int front() const;
    int back() const;

    int* operator[](int idx);        // non-const, returns pointer
    int operator[](int idx) const;   // const, returns value

    void push_back(int val);
    void pop_back();

    Iter begin();
    Iter end();
};
```

This should compile and run using only already-landed compiler features.

## Phases

### Phase 0 (this slice)

- Write `fixed_vec_smoke.cpp` acceptance test
- Verify it compiles and runs correctly
- Fix any compiler bugs that surface
- No new language features expected — this is a validation milestone

## Test Cases

- `fixed_vec_smoke.cpp` — comprehensive vector-like container smoke test

## Success Criterion

- The smoke test passes without compiler changes (ideal)
- Or: minimal, targeted compiler fixes make it pass
- Full test suite remains at baseline or above
