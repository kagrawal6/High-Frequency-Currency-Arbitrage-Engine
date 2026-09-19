# OpenMP tests

`make test` builds and runs `tests/test_engine`.

Coverage:

1. Currency IDs and pair count
2. Consistent cross (no cycle)
3. Planted triangle
4. 20% product-loop profit band
5. mmap CSV fixture
6. `threadCount()` binding
7. **OpenMP @ max threads reproduces the serial opportunity list** on a 400-timestamp live slice (timestamp, profit, cycle)

15 assertions, 0 failures on the close-out run.
