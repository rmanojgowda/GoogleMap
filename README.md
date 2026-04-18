# GMap — India Road Network & Algorithm Visualizer

> A production-grade graph routing engine built in C++ with a Google Maps-style frontend.
> Implements Dijkstra, A*, Bidirectional Dijkstra, and Bi-A* on real OpenStreetMap data.

![CI](https://github.com/rmanojgowda/GoogleMap/actions/workflows/ci.yml/badge.svg)

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

Four algorithms implemented so their efficiency can be compared directly.

---

## Architecture

```
GMap/
├── include/
│   ├── Types.h              ← Shared structs: City, Edge, AlgorithmStats
│   ├── Graph.h              ← Adjacency list interface
│   ├── Algorithms.h         ← Dijkstra | A* | BiDi | Bi-A* | TSP
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
├── .github/workflows/
│   └── ci.yml               ← GitHub Actions CI pipeline
├── data/
│   └── india_map_data.js    ← Frontend dataset
├── parse_osm.py             ← OSM PBF → C++ + JS pipeline
├── GMap_v3.html             ← Full frontend (Leaflet + CartoDB)
└── CMakeLists.txt
```

---

## Algorithms

### 1. Dijkstra's Shortest Path

**Time:** O((V + E) log V) | **Space:** O(V)

Explores cities in order of distance from source using a min-heap. Guaranteed optimal. Search expands like a circle in all directions.

**Why adjacency list?** The graph is sparse (~3.5 edges per city). An adjacency matrix for 268 cities wastes 71,824 cells, 99.4% empty. Adjacency list uses exactly O(V + E) entries.

---

### 2. A* (A-Star)

**Time:** O((V + E) log V) | **Practical:** 16-18% fewer nodes on OSM graph

Adds a heuristic h(n) to guide search toward the destination:

```
f(n) = g(n) + h(n)
         |       |
   actual road   Haversine straight-line
   dist so far   dist to destination
```

**Why Haversine?** India spans 30° latitude. Flat Euclidean distance is inaccurate at this scale. Haversine computes exact great-circle distance and never overestimates (admissible).

**Safety factor 0.5** applied for OSM data where some edges have road distance shorter than straight-line (ferry routes, GPS corrections). Correctness over performance.

---

### 3. Bidirectional Dijkstra

**Time:** O((V + E) log V) | **Practical:** 14% fewer nodes

Two simultaneous Dijkstra searches from both ends meeting in the middle. Each covers radius d/2, total area ≈ half of one-way.

**Correct termination condition:**
```cpp
if (topF + topB >= best) break;
```
A subtle bug in many BiDi implementations — found and fixed during exhaustive correctness testing.

---

### 4. Bi-A* (Bidirectional A-Star)

**Time:** O((V + E) log V) | **Practical:** 18-23% fewer nodes — best of all algorithms

Combines bidirectional search with heuristic guidance. Uses the **average heuristic** to ensure consistency:

```
p_forward(n)  = (h(n→dst) - h(n→src)) / 2
p_backward(n) = (h(n→src) - h(n→dst)) / 2
```

**Why average heuristic?** Plain Bi-A* with separate forward/backward heuristics violates consistency at the meeting point — one search prices a node higher while the other prices it lower, causing premature termination and suboptimal paths. The average heuristic is symmetric: p_F(n) + p_B(n) = 0 for all n, restoring correctness.

**Benchmark — Kochi → Amritsar (longest India route):**
```
Dijkstra   260 nodes  (baseline)
A*         243 nodes  ( 6% fewer)
BiDi       222 nodes  (14% fewer)
Bi-A*      198 nodes  (23% fewer)  ← wins on long routes
```

---

### 5. Road Trip Planner (TSP)

TSP is NP-hard (O(n!) exact). Two heuristics combined:
- **Nearest Neighbour O(n²)** — greedy construction, within 20-25% of optimal
- **2-opt local search** — reverses sub-sequences to eliminate crossings, cuts another 5-15%

---

## Scale Testing Results

| Scale | Cities | Roads | Dijkstra | A* | BiDi | Bi-A* |
|---|---|---|---|---|---|---|
| Small | 31 | 56 | 18 | 14 | 17 | 15 |
| Large | 268 | 465 | 140 | 108 | 110 | 95 |
| OSM | 5,000 | 9,524 | 2290 | 1804 | 1870 | 1750 |

200 random queries per scale. **0 correctness mismatches** across all scales and all algorithms.

**Key insight:** Bi-A* consistently explores the fewest nodes, especially on long routes where heuristic guidance AND geometric halving both contribute.

---

## Bottleneck Analysis

Phase-level profiling, 268-city graph, 300 queries:

**Dijkstra — 18 µs per query**
```
Heap operations    47%  ← primary bottleneck
Edge relaxation    51%
Initialisation      1%
Path reconstruct    1%
```

**A* — 26 µs per query**
```
Edge relaxation    41%
Heuristic cost     39%  ← cost of guidance
Heap operations    19%
```

**Production optimisations:**

| Optimisation | Speedup | Why |
|---|---|---|
| Contraction Hierarchies | ~1000x | Offline shortcuts skip unimportant nodes |
| Fibonacci Heap | 2-3x | O(1) decrease-key vs O(log n) |
| Heuristic caching | 10-20% | Avoid recomputing h per neighbour |
| Pre-allocated arrays | 5-10% | Avoid malloc per query |

---

## Stress Test Results — 35/35 passing

- 1000 random queries: 0 correctness mismatches in 31ms
- All 930 city pairs (31-city graph): Dijkstra = A* = BiDi = Bi-A* exactly
- All paths symmetric: dist(A→B) = dist(B→A)
- **Bug found and fixed:** Missing bounds check in `shortestPath`
- **Bug found and fixed:** BiDi wrong termination condition

---

## CI/CD Pipeline

Every push to main automatically:
1. Builds all 5 executables on Ubuntu
2. Runs 118 unit tests
3. Runs 35 stress tests
4. Runs scale benchmarks

Powered by GitHub Actions. See `.github/workflows/ci.yml`.

---

## Data Pipeline

```
india-260327.osm.pbf (1.6 GB real OSM data)
         │
         ▼  parse_osm.py  (~80 minutes)
         │  Pass 1: 284,846 place nodes → 5,000 kept
         │  Pass 2: 8,975 road edges extracted
         │  Spatial grid index for O(1) nearest-city lookup
         │  549 proximity edges added (max 150km cap)
         │  Road dist = max(accumulated, haversine) × 1.35
         │
         ├──► MapLoader_OSM.h     (C++ — 14,543 lines)
         └──► india_osm_data.js   (JavaScript frontend)
```

---

## How Google Maps Works

Google Maps uses **Contraction Hierarchies (CH)** on top of Bi-A*:

**Offline preprocessing (done once, takes days):**
- Rank every node by importance (how many shortest paths pass through it)
- Remove low-importance nodes and add shortcut edges that preserve paths
- Store contracted graph permanently

**Online query (microseconds):**
- Run bidirectional A* upward through importance hierarchy only
- Result: 1 billion nodes, 10ms query time

This project implements the A* + Bidirectional + Bi-A* foundation. CH preprocessing is the remaining gap — it requires days of offline compute but gives ~1000x speedup.

---

## Building

```bash
mkdir build && cd build
cmake .. -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release
cmake --build .
```

```bash
.\gmap.exe          # CLI: choose 31 / 268 / 5000 city map
.\gmap_tests.exe    # 118 unit tests
.\scale_test.exe    # Benchmarks across all 3 datasets
.\stress_test.exe   # 35 stress tests
.\bottleneck.exe    # Phase-level profiler
```

---

## Key Design Decisions

**Why C++?** Direct memory control, no GC pauses, same language as production routing at Google, HERE, TomTom.

**Why adjacency list?** Sparse graph. Matrix wastes O(V²), 99% empty.

**Why Haversine for heuristic?** Earth curvature matters at India scale (30° latitude span). Always admissible.

**Why average heuristic for Bi-A*?** Ensures consistency in both directions — prevents premature termination at meeting point.

**Why NN + 2-opt for TSP?** Exact TSP is O(n!). NN + 2-opt gives within 5-15% of optimal in O(n²).

---

## Complexity Summary

| Operation | Time | Space |
|---|---|---|
| Shortest path (all 4 algorithms) | O((V+E) log V) | O(V) |
| Road trip (NN + 2-opt) | O(n² × (V+E) log V) | O(n²) |
| Build graph | O(1) amortised | O(V+E) |
| City lookup | O(1) average | — |

---

## Design at Scale — How to Route Across 500 Million Nodes

*This section answers the interview question: "Your engine works on 5,000 cities.
How would you redesign it to power a real navigation system at Google Maps scale?"*

---

### The Problem at Scale

Our current engine on 5,000 OSM cities:
```
Query time     : ~300 µs
Nodes explored : ~2,300 (Dijkstra), ~1,800 (Bi-A*)
Memory         : ~2 MB
```

Google Maps has ~500 million road nodes globally:
```
Target query time  : < 10ms
Nodes to explore   : 500,000,000
Naive Dijkstra     : hours per query
```

Three fundamental problems to solve:
1. **Speed** — Dijkstra at this scale is too slow
2. **Memory** — the full graph won't fit in one machine's RAM
3. **Freshness** — road conditions change in real time

---

### Layer 1 — Contraction Hierarchies (Speed, ~1000x)

**The insight:** Not all nodes matter equally for long routes.

Rank every node by *importance* — roughly, how many shortest paths pass
through it. Delhi and Mumbai are extremely important (millions of routes
pass through them). A village in rural Rajasthan is unimportant.

**Offline preprocessing (once, takes days):**
```
1. Remove least important node V
2. For every pair (u, w) where u→V→w was the shortest path:
   Add shortcut edge u→w with weight d(u,V) + d(V,w)
3. Repeat for all nodes in importance order
```

The shortcut edges "remember" paths through removed nodes.

**Online query (microseconds):**
```
Forward  search from source    : only expand to HIGHER importance nodes
Backward search from destination: only expand to HIGHER importance nodes
Meet at the highest importance node on the optimal path
```

Result: instead of exploring millions of nodes, each query touches ~100-200
high-importance nodes. This is how Google gets < 10ms on 500M nodes.

**Why we don't implement it in GMap:**
CH preprocessing requires days of compute and gigabytes of precomputed
shortcut edges. It's an offline infrastructure problem, not an algorithm
problem. The online query is literally bidirectional A* — which we already have.

---

### Layer 2 — Graph Partitioning (Memory, distributed)

A 500M node graph with edge weights won't fit in one machine's RAM.

**Approach: geographic sharding**
```
Partition India into regions (e.g., state-level)
Each region's graph lives on a separate server
Border nodes connect regions with cross-shard edges

Query Delhi → Kanyakumari:
  1. Identify which shards the path crosses
  2. Run CH within each shard
  3. Stitch results at border nodes
```

This mirrors how Google's infrastructure works — routing is a distributed
computation across hundreds of servers, not a single machine.

**Alternative: METIS graph partitioning**
Minimises cross-partition edges (reduces inter-server communication).
Used by Facebook's social graph, Google's web graph, and real routing engines.

---

### Layer 3 — Real-Time Traffic (Freshness)

CH precomputes shortest paths assuming static edge weights.
Traffic changes edge weights in real time — reopening a road, or
a traffic jam slowing NH44 from 80km/h to 15km/h.

**The problem:** Rerunning full CH preprocessing every minute is impossible
(takes days). You can't invalidate a 1000x precomputed speedup every time
there's a fender bender.

**Google's actual solution: two-layer routing**
```
Static layer  : CH-preprocessed base graph (updated nightly)
Dynamic layer : Traffic overlay applied on top of static routes
```

For a query Delhi → Mumbai with live traffic:
```
1. CH gives base route in microseconds (ignores traffic)
2. Traffic service checks each edge of that route for congestion
3. If congestion found → re-route affected segment only
4. Return adjusted ETA
```

Traffic data comes from 1B+ Android phones reporting GPS speed.
Each phone is a passive sensor — no user action needed.

**In our system:** We'd add an `edge_weight_multiplier[u][v]` lookup
that the routing algorithm reads before each edge relaxation:
```cpp
double effective_weight = base_weight * traffic_service.getMultiplier(u, v);
```
This is the engineering abstraction — the algorithm doesn't change,
only the weights it reads.

---

### Layer 4 — Turn Costs (Correctness)

Our graph treats roads as undirected edges with distance weights.
Real routing requires:

```
Left turns  : +30 seconds (cross traffic, wait for gap)
U-turns     : +120 seconds (often illegal)
Right turns : +5 seconds (usually free)
Traffic lights: variable delay
```

**Implementation:** Replace node-based graph with edge-based graph.
Each directed edge (u→v) becomes a node. Turning from edge (u→v) to
edge (v→w) has cost = turn_penalty(u,v,w).

This doubles the graph size but makes routing significantly more accurate,
especially in dense urban areas where turn penalties dominate travel time.

---

### Layer 5 — Multimodal Routing

Google Maps combines road + metro + bus + walking in one query.

**The abstraction:** Every transit stop is a node. Every route (road/rail/bus)
is an edge with time-based weight. A car edge from A to B costs distance/speed.
A metro edge from station X to Y costs fixed_travel_time + wait_time.

**Time-dependent edges:** A metro edge at 8am has weight 3 minutes.
The same edge at 3am has weight "closed" (infinite). Dijkstra handles
this by parameterising edge weights on departure time:
```cpp
double weight = edge.getWeight(current_time + accumulated_travel_time);
```

---

### Full System Architecture at Scale

```
User query: "Delhi → Mumbai"
      │
      ▼
Load Balancer
      │
      ▼
Routing Service (stateless, horizontally scaled)
      │
      ├──► Graph Server (CH-preprocessed, sharded by region)
      │         Returns base route in microseconds
      │
      ├──► Traffic Service (real-time edge weights from phones)
      │         Adjusts route for current conditions
      │
      ├──► ETA Service (ML model on historical travel times)
      │         Predicts arrival time, not just distance
      │
      └──► Cache Layer (Redis/Memcached)
                Popular routes cached (Delhi→Mumbai queried 10M times/day)
                Cache hit → skip routing entirely → microseconds

Response: route + ETA + traffic warnings
```

---

### The Gap Between GMap and This

| Layer | GMap | Production |
|---|---|---|
| Core routing | Bi-A* ✅ | CH + Bi-A* |
| Graph size | 5,000 nodes | 500,000,000 nodes |
| Preprocessing | None | Days of offline CH |
| Sharding | Single machine | Hundreds of servers |
| Traffic | Static weights | Real-time from 1B phones |
| Turn costs | Not implemented | Full turn penalty model |
| Multimodal | Road only | Road + Rail + Bus + Walk |
| Caching | None | Redis, 10M popular routes |

GMap implements the algorithmic core correctly — the production gap is
infrastructure and data pipeline, not algorithms. The routing logic
(Dijkstra, A*, Bi-A*) is identical; it just runs on a larger, richer graph.


---

## What's Missing vs Production

| Feature | GMap | Google Maps |
|---|---|---|
| Core algorithm | Bi-A* + Bidirectional | Contraction Hierarchies + Bi-A* |
| Graph size | 5,000 nodes | 1,000,000,000+ nodes |
| Preprocessing | None | Weeks offline |
| Traffic | No | Real-time 1B+ phones |
| Turn costs | No | Left turn, U-turn penalties |

---

*Built with C++17 · CMake · Python (osmium) · Leaflet.js · OpenStreetMap · CartoDB*
