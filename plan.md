# Workflow:

1. run test

```bash
rm -rf build
# build
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug -DENABLE_LLVM_GCC_C_TORTURE_TESTS=ON
cmake --build build -j4
# test
ctest -j --test-dir build --output-on-failure > test_fail.log

```

2. pickup a test fail case to fix
order: FRONTEND_FAIL > BACKEND_FAIL > RUNTIME_FAIL. 

testcase timeout policy:
- default per-test timeout: 30s
- any >30s test runtime treated as suspicious regression unless explicitly documented

3. All test before submit
```bash
ctest -j --test-dir build --output-on-failure > test_fail.log
```


4. git commit
如果沒有打壞既有測試，且pass數量增加，那才可遞交commit，將更新內容寫在commit message