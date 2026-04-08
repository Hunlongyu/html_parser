# Benchmark Report 2026-04-08

## Scope

- Project: hps / html_parser
- Date: 2026-04-08
- Platform: Windows x64
- Compiler toolchain: MSVC 19.50 via Visual Studio 2026 Insiders
- Build mode: Release only
- Build tree: `out/build/x64-Release`

Debug numbers were collected earlier only to validate the new benchmark harness and are intentionally discarded here.

## Method

### Tokenizer

- Target: `tokenizer_bench`
- Input: synthetic HTML payloads of roughly 8 KiB / 64 KiB / 512 KiB / 2 MiB
- Metric: average latency, tail latency, throughput in MiB/s

### Parser

- Target: `parser_bench`
- Input: bundled sample files under `examples/html`
- Metric: average latency, throughput in MiB/s, result count as DOM node count

### CSS Selectors

- Target: `css_selector_bench`
- Input: synthetic DOM around 512 KiB
- Metric: selector-set parse cost and per-selector match cost

## Results

### Tokenizer

| Scenario | Input Bytes | Iterations | Tokens | Avg ms | P95 ms | Throughput MiB/s |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| synthetic_8k | 8,233 | 400 | 611 | 0.178575 | 0.258500 | 43.968084 |
| synthetic_64k | 65,791 | 200 | 4,851 | 1.068210 | 1.644100 | 58.736726 |
| synthetic_512k | 524,626 | 60 | 38,651 | 9.077972 | 11.115500 | 55.113891 |
| synthetic_2048k | 2,097,154 | 12 | 154,491 | 44.545850 | 46.501400 | 44.897603 |

### Parser

| Sample | Input Bytes | Iterations | DOM Nodes | Avg ms | P95 ms | Throughput MiB/s |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| base.html | 886 | 400 | 19 | 0.016344 | 0.018000 | 51.698204 |
| complex.html | 10,654 | 200 | 93 | 0.090939 | 0.108700 | 111.727535 |
| fofa.html | 1,260,998 | 12 | 4,713 | 18.466200 | 20.948000 | 65.123382 |
| panlong.html | 158,957 | 60 | 538 | 0.711907 | 0.780800 | 212.939723 |
| shodan.html | 407,783 | 60 | 492 | 1.117832 | 1.412000 | 347.898691 |

### CSS Selector Parsing

| Scenario | Input Bytes | Iterations | Selector Count | Avg ms | P95 ms | Throughput MiB/s |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| selector_set | 204 | 500 | 14 | 0.013196 | 0.017500 | 14.742847 |

### CSS Selector Matching

| Selector | Input Bytes | Iterations | Match Count | Avg ms | P95 ms | Throughput MiB/s |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| div | 534,247 | 160 | 1,065 | 0.211046 | 0.234100 | 2414.151602 |
| .container | 534,247 | 160 | 355 | 0.283383 | 0.440800 | 1797.914982 |
| #main-content | 534,247 | 160 | 355 | 0.201661 | 0.233400 | 2526.510282 |
| [type='text'] | 534,247 | 160 | 355 | 0.201920 | 0.224600 | 2523.264870 |
| a[href='#'] | 534,247 | 160 | 1,065 | 0.254234 | 0.299100 | 2004.051950 |
| div > p | 534,247 | 160 | 0 | 0.169474 | 0.200400 | 3006.351382 |
| ul li a | 534,247 | 160 | 1,065 | 0.275150 | 0.504500 | 1851.708677 |
| h1 + nav | 534,247 | 160 | 355 | 0.190254 | 0.277200 | 2677.990013 |
| li:first-child | 534,247 | 160 | 710 | 0.239829 | 0.272400 | 2124.422708 |
| li:nth-child(2) | 534,247 | 160 | 710 | 0.273927 | 0.303700 | 1859.976837 |
| input:checked | 534,247 | 160 | 355 | 0.214043 | 0.334400 | 2380.350420 |
| button:disabled | 534,247 | 160 | 355 | 0.192182 | 0.214000 | 2651.122238 |
| div.container > header nav ul li a.active | 534,247 | 160 | 355 | 0.245665 | 0.287100 | 2073.952914 |
| section.section h2 + p.description | 534,247 | 160 | 355 | 0.211985 | 0.244800 | 2403.460823 |

## Readout

### What these numbers say

- The tokenizer now lands roughly in the 45-59 MiB/s band on this Windows/MSVC Release build.
- Real-document parser throughput is highly shape-dependent. On the bundled samples it spans roughly 65-348 MiB/s.
- The slowest bundled parse case here is `fofa.html`, which is also the heaviest DOM-building case in this run.
- CSS selector matching on the synthetic 512 KiB DOM is cheap in absolute latency terms, mostly around 0.17-0.28 ms per selector.

### What these numbers do not say

- They are not cross-library apples-to-apples results.
- They do not include memory usage, allocator pressure, or peak RSS.
- They do not include pathological malformed HTML or adversarial selectors.

## Comparison To Other Libraries

### Lexbor

Public positioning:

- Actively maintained.
- Describes itself as one of the fastest HTML parsers available.
- Ships HTML, CSS, selectors, and encoding support in one ecosystem.
- States production testing on 200+ million HTML pages.
- Repository now contains a dedicated `benchmarks/` tree, including HTML tokenizer and selector benchmark programs.
- The project article "Part One - HTML" publishes one historical parser figure: about `235 MB/s` average on Amazon pages, on a single core of a 2012 i7.

Practical interpretation:

- Lexbor is the strongest reference point for absolute throughput in this space.
- Given its pure-C design, custom memory management, and long-term optimization focus, it is the library most likely to beat hps on both raw parse speed and memory efficiency under the same harness.
- If the goal is to claim performance leadership, Lexbor is the one that must be beaten in a same-corpus benchmark.
- The public `235 MB/s` number is useful as evidence that Lexbor does publish at least some performance data, but it is not directly comparable to the hps numbers above because the corpus, machine, compiler flags, and exact benchmark method differ.

### MyHTML

Public positioning:

- Older fast pure-C HTML5 parser.
- README still links to a historical benchmark article and benchmark code.
- README now explicitly tells users to prefer Lexbor.
- Last tagged release is old, and the project is no longer the author's recommended path.

Practical interpretation:

- MyHTML is still a relevant historical speed reference, but not the best forward-looking baseline anymore.
- If hps is competitive with MyHTML on a same-corpus benchmark, that is good news; it still would not settle the question against Lexbor.

### Gumbo

Public positioning:

- Archived and read-only as of 2026-01-21.
- README says the project has been unmaintained since 2016 and should not be used.

Practical interpretation:

- Gumbo is no longer a sensible target for a modern performance claim.
- It can still serve as a historical correctness/compatibility reference, but not as the main competitive benchmark for a new parser.

## Current Conclusion

- hps is already in the range where it is worth benchmarking seriously; this is not a trivially slow prototype anymore.
- The current benchmark setup is now good enough for repeatable in-project regression tracking.
- For external marketing-style comparison, the next meaningful step is a same-harness benchmark against Lexbor first, then optionally MyHTML, while keeping Gumbo as a legacy reference only.

## Recommended Next Step

1. Add a separate `benchmark_compare` target that runs the same corpus through hps and Lexbor.
2. Add memory metrics alongside latency and throughput.
3. Add at least one malformed HTML corpus and one large selector stress case.