# Sequential tests

`make test` builds and runs `tests/test_engine`.

Coverage:

1. Stable currency IDs
2. Empty graph
3. Bid/ask rate lookup
4. Consistent cross (no cycle)
5. Planted triangle (cycle + profit > 0)
6. Wide-spread graph stays numerically stable
7. Profit formula on a 20% product loop
8. mmap CSV fixture
9. Packed detector ≡ ForexGraph detector
10. PositionsManager books a planted cycle
11. Live 200-timestamp slice is deterministic

34 assertions, 0 failures on the close-out run.
