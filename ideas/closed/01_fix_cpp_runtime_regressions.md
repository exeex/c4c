Status: Complete

# Fix C++ Runtime Regressions

## Goal

Restore the failing internal C++ positive runtime tests:

- `cpp_positive_sema_scoped_enum_bitwise_runtime_cpp`
- `cpp_positive_sema_template_angle_bracket_validation_cpp`

## Scope

- fix enum runtime codegen so scoped enums with fixed underlying types return the correct LLVM width
- fix callable signature lowering so template alias member typedefs such as `enable_if_t<...>` resolve before codegen
- validate with the two targeted tests and a nearby regression slice

## Notes

- current failure mode for both tests is missing output binary after invalid LLVM IR reaches the host compiler
- this is a regression-fix task, not a broader feature expansion

## Completed

- resolved enum integer coercion so fixed underlying widths are preserved in LLVM integer casts and returns
- normalized zero-sized helper-struct template instantiations to the scalar type actually returned by the function body
- validated the original two failures, nearby runtime coverage, and the full `ctest --test-dir build -j --output-on-failure` suite
