# CUDA tests

`make test` builds and runs `tests/test_engine`.

Coverage:

1. `__host__` `detectOne` matches `detectArbitrage` on a planted 20% loop
2. Consistent cross → `cycle_len == 0`
3. `cudaBackendName()` is non-empty (`cuda` or `cuda-emu`)
4. Packed `launchDetectKernel` matches `detectArbitrageAt` on a 300-timestamp live slice

8 assertions, 0 failures on the close-out run.
