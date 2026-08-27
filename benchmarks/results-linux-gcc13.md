# Illustrative benchmark — 2026-08-27

These results were produced by `fastmc-benchmark` from this repository. They
are a reproducible measurement on one machine, not a universal performance
claim.

- CPU: Intel Xeon Platinum 8370C @ 2.80 GHz
- Available logical CPUs: 9
- Compiler: GCC 13.3.0
- Build: CMake `Release`
- Product: European call, antithetic sampling

| Raw paths | Threads | Seconds | Raw paths/second | Price |
| ---: | ---: | ---: | ---: | ---: |
| 100,000 | 1 | 0.005081 | 19,679,430 | 10.433052 |
| 100,000 | 2 | 0.013332 | 7,500,504 | 10.415876 |
| 100,000 | 4 | 0.005319 | 18,801,484 | 10.390389 |
| 1,000,000 | 1 | 0.041719 | 23,969,731 | 10.439327 |
| 1,000,000 | 2 | 0.033930 | 29,472,046 | 10.458704 |
| 1,000,000 | 4 | 0.021704 | 46,074,700 | 10.463355 |

The 100,000-path case shows normal short-run scheduling noise and thread
overhead. The larger case is more useful for assessing scaling.
