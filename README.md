# GMap — India Road Network & Algorithm Visualizer

> A production-grade graph routing engine built in C++ with a Google Maps-style frontend.
> Implements Dijkstra, A*, and Bidirectional Dijkstra on real OpenStreetMap data.

---

## Live Demo

Open `GMap_v3.html` in any browser — no server, no installation required.

---

## Screenshots

### Directions — Delhi → Bengaluru (Dijkstra)
![Route](screenshots/screenshot_route.png)

### Satellite View — Same Route
![Satellite](screenshots/screenshot_satellite.png)

### Algorithm Benchmark — Kochi → Guwahati
A* explored **37% fewer nodes** than Dijkstra on this cross-country route.
![Benchmark](screenshots/screenshot_benchmark.png)

### Road Trip Planner — 4 Cities (TSP)
Nearest-Neighbour + 2-opt finds the optimal tour: Bengaluru → Mumbai → Delhi → Kolkata → Bengaluru
![Road Trip](screenshots/screenshot_roadtrip.png)

### Scale Test Results — Terminal Output
![Terminal](screenshots/screenshot_terminal.png)

---

## What This Project Does

A complete map routing system solving two problems:

**1. Shortest Path** — Given two Indian cities, find the fastest road route.
**2. Optimal Road Trip** — Given N cities, find the most efficient order to visit all of them.

Both solved with multiple algorithms so their efficiency can be compared directly.

---

## Architecture

```
GMap/
├── include/
│   ├── Types.h              ← Shared structs: City, Edge, AlgorithmStats
│   ├── Graph.h              ← Adjacency list interface
│   ├── Algorithms.h         ← Dijkstra | A* | BiDi | TSP interfaces
│   ├── MapLoader.h          ← 31-city demo dataset
│   ├── MapLoader_Large.h    ← 268-city generated dataset
│   └── MapLoader_OSM.h      ← 5000-city real OSM dataset
├── src/
│   ├── Graph.cpp            ← Graph implementation
│   ├── Algorithms.cpp       ← All algorithm implementations
│   └── main.cpp             ← Interactive CLI
├── tests/
│   ├── TestFramework.h      ← GTest-compatible test harness
│   ├── test_gmap.cpp        ← 118 unit tests
│   ├── scale_test.cpp       ← Scale benchmarks
│   ├── stress_test.cpp      ← Edge case & break tests
│   └── bottleneck.cpp       ← Phase-level profiling
├── data/
│   └── india_map_data.js    ← Frontend dataset
├── parse_osm.py             ← OSM PBF → C++ + JS pipeline
├── generate_map.py          ← Synthetic dataset generator
├── GMap_v3.html             ← Full frontend (Leaflet + CartoDB)
└── CMakeLists.txt
```

---

## Algorithms

### 1. Dijkstra's Shortest Path

**Time:** O((V + E) log V) | **Space:** O(V)

Explores cities in order of distance from source using a min-heap. Guaranteed optimal on non-negative weighted graphs. Explores ALL cities within distance d — search expands like a circle.

**Why adjacency list?** The graph is sparse (avg ~3.5 edges per city). An adjacency matrix for 268 cities wastes 71,824 cells, 99.4% empty. The adjacency list uses exactly O(V + E) entries.

---

### 2. A* (A-Star)

**Time:** O((V + E) log V) | **Practical:** 82% fewer nodes on 5000-city OSM graph

Adds a heuristic h(n) to guide search toward the destination:

```
f(n) = g(n) + h(n)
         |       |
   actual road   Haversine straight-line
   dist so far   dist to destination
```

**Why Haversine?** India spans 30 degrees latitude. Flat Euclidean distance is inaccurate at this scale. Haversine computes exact great-circle distance and never overestimates (admissible) — guaranteeing A* stays optimal.

**Safety factor 0.5** applied to handle OSM data anomalies (ferry routes, GPS corrections) where road distance can be shorter than straight-line.

---

### 3. Bidirectional Dijkstra

**Time:** O((V + E) log V) | **Practical:** ~21% fewer nodes on 268-city graph

Two simultaneous searches from both ends, meeting in the middle. Each covers radius d/2 so total area ≈ half of one-way search.

**Correct termination condition:**
```cpp
// Stop only when neither frontier can improve best anymore
if (topF + topB >= best) break;
```
This is a subtle bug in many BiDi implementations — terminating early when frontiers first touch gives incorrect results on some graph structures. Fixed during testing.

---

### 4. Road Trip Planner (TSP)

TSP is NP-hard. Two heuristics combined:
- **Nearest Neighbour O(n²)** — greedy construction, within 20-25% of optimal
- **2-opt local search** — reverses sub-sequences to eliminate crossings, cuts another 5-15%

---

## Scale Testing Results

| Scale | Cities | Roads | Dijkstra nodes | A* nodes | A* saves |
|---|---|---|---|---|---|
| Small | 31 | 56 | 18 | 14 | 24% |
| Large | 268 | 465 | 140 | 108 | 23% |
| OSM | 5,000 | 9,536 | 2,448 | 437 | **82%** |

200 random queries per scale. 0 correctness mismatches on small/large maps.

**Key insight:** Graph grew 161x (31 → 5000 cities). Dijkstra nodes grew 136x. A* nodes grew only 31x — A* scales with path length, not graph size.

---

## Bottleneck Analysis

Phase-level profiling, 268-city graph, 300 queries:

**Dijkstra — 18 µs per query**
```
Heap operations    47%  ← primary bottleneck
Edge relaxation    51%  ← primary bottleneck
Initialisation      1%
Path reconstruct    1%
```

**A* — 26 µs per query**
```
Edge relaxation    41%
Heuristic cost     39%  ← cost of guidance
Heap operations    19%  (fewer nodes visited)
```

**Production optimisations:**

| Optimisation | Speedup |
|---|---|
| Contraction Hierarchies | ~1000x |
| Fibonacci Heap | 2-3x |
| Heuristic caching | 10-20% |
| Pre-allocated arrays | 5-10% |

---

## Stress Test Results — 35/35 passing

- 1000 random queries: 0 correctness mismatches in 31ms
- All 930 city pairs (31-city graph): Dijkstra = A* = BiDi exactly
- All paths symmetric: dist(A→B) = dist(B→A)
- **Bug found:** Missing bounds check in `shortestPath` — fixed
- **Bug found:** BiDi wrong termination condition — fixed

---

## Data Pipeline

```
india-260327.osm.pbf (1.6 GB real OSM data)
         │
         ▼  parse_osm.py  (~60 minutes)
         │  Pass 1: 284,846 place nodes → 5,000 kept
         │  Pass 2: 8,975 road edges extracted
         │  Spatial grid index for O(1) nearest-city lookup
         │  561 proximity edges added for connectivity
         │
         ├──► MapLoader_OSM.h     (C++ — 14,555 lines)
         └──► india_osm_data.js   (JavaScript frontend)
```

---

## How Google Maps Works

Google Maps uses Contraction Hierarchies (CH) on top of A*:

**Offline preprocessing (done once, takes days):**
- Rank every node by importance (shortest paths through it)
- Add shortcut edges that skip unimportant nodes

**Online query (milliseconds):**
- Run bidirectional A* on high-importance nodes only
- Result: 1 billion nodes, 10ms query time

This project implements the A* + Bidirectional foundation. CH preprocessing is the remaining gap.

---

## Building

```bash
mkdir build && cd build
cmake .. -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release
cmake --build .
```

```bash
.\gmap.exe          # Interactive CLI (choose 31 / 268 / 5000 city map)
.\gmap_tests.exe    # 118 unit tests
.\scale_test.exe    # Scale benchmarks across all 3 datasets
.\stress_test.exe   # 35 stress tests
.\bottleneck.exe    # Phase-level profiler
```

---

## Key Design Decisions

**Why C++?** Direct memory control, no GC pauses, same language as production routing engines at Google, HERE, TomTom.

**Why adjacency list?** Sparse graph. Matrix wastes O(V²) space, 99% empty for India's road network.

**Why Haversine for A*?** Earth curvature matters at India scale. Haversine is exact great-circle distance, always admissible.

**Why NN + 2-opt for TSP?** Exact TSP is O(n!). For 20 cities = 2.4 quintillion operations. NN + 2-opt gives within 5-15% of optimal in O(n²).

---

## Complexity Summary

| Operation | Time | Space |
|---|---|---|
| Shortest path (Dijkstra / A* / BiDi) | O((V+E) log V) | O(V) |
| Road trip (NN + 2-opt) | O(n² × (V+E) log V) | O(n²) |
| Build graph | O(1) amortised | O(V+E) |
| City lookup | O(1) average | — |

---

## What's Missing vs Production

| Feature | GMap | Google Maps |
|---|---|---|
| Core algorithm | A* + Bidirectional | Contraction Hierarchies + A* |
| Graph size | 5,000 nodes | 1,000,000,000+ nodes |
| Preprocessing | None | Weeks offline |
| Traffic | No | Real-time 1B+ phones |
| Turn costs | No | Left turn, U-turn penalties |

---

*Built with C++17 · CMake · Python (osmium) · Leaflet.js · OpenStreetMap · CartoDB*