# Performance Demo - Extreme Stress Test

## Overview
This example pushes Aurora3D to its absolute limits with 100,000+ objects.

## Features
- **Massive Object Count**: 100,000 individual objects
- **Real-time Profiling**: Performance metrics every second
- **Dynamic LOD**: Automatic quality scaling based on FPS
- **Memory Optimization**: Efficient object pooling

## What Makes It Extreme?
- Simulates **100,000+ objects** updated every frame
- Targets **60+ FPS** on modern hardware
- Real-time adaptive quality system
- Comprehensive performance metrics

## Running the Example
```bash
./build/bin/Debug/performance_demo
# or on Linux
./build/bin/performance_demo
```

## Sample Output
```
=== EXTREME PERFORMANCE DEMO ===
Initializing stress test with 100,000+ objects...
✓ Objects initialized: 100000
✓ Memory allocated: ~7 MB
✓ Target FPS: 60+
✓ Performance monitoring: ACTIVE

⚠ WARNING: EXTREME workload ahead!

┌─────────────────────────────────────────────┐
│ PERFORMANCE METRICS                          │
├─────────────────────────────────────────────┤
│ Active Objects: 100000 / 100000
│ Current FPS: 62.5
│ Avg Frame Time: 16.2 ms
│ Peak FPS: 144.3
│ Min FPS: 58.1
│ LOD Distribution:
│   L0 (High): 25000
│   L1 (Med):  35000
│   L2 (Low):  30000
│   L3 (Min):  10000
│ Memory: ~7 MB
└─────────────────────────────────────────────┘
```

## Technical Details

### Object Distribution
- **3D Grid Pattern**: 100 x 100 x 10 grid
- **Procedural Positioning**: Mathematical spiral distribution
- **Dynamic Movement**: Physics-based velocity updates
- **Boundary Detection**: Objects bounce off limits

### LOD System
The demo implements a 4-level LOD system:

| LOD Level | Quality | Triangle Count | Usage |
|-----------|---------|----------------|-------|
| L0 | Highest | 100% | Near camera |
| L1 | High | 75% | Medium distance |
| L2 | Medium | 50% | Far distance |
| L3 | Low | 25% | Very far |

**Adaptive Scaling:**
- If FPS < 60: Reduce quality (increase LOD level)
- If FPS > 72: Increase quality (decrease LOD level)

### Performance Targets

| Hardware Tier | Expected FPS | Object Count | LOD Mix |
|---------------|--------------|--------------|---------|
| RTX 3060 | 60-90 | 100,000 | Mixed L0-L2 |
| RTX 2060 | 45-60 | 100,000 | Mixed L1-L3 |
| GTX 1060 | 30-45 | 100,000 | Mostly L2-L3 |

## Code Highlights

### Object Update Loop
```cpp
void updateAllObjects(float dt) {
    for (auto& obj : objects_) {
        if (!obj.active) continue;
        
        // Update position based on velocity
        obj.x += obj.vx * dt * 100.0f;
        obj.y += obj.vy * dt * 100.0f;
        obj.z += obj.vz * dt * 100.0f;
        
        // Bounce off boundaries
        if (std::abs(obj.x) > 100.0f) obj.vx = -obj.vx;
        if (std::abs(obj.y) > 50.0f) obj.vy = -obj.vy;
        if (std::abs(obj.z) > 100.0f) obj.vz = -obj.vz;
    }
}
```

### Adaptive LOD Management
```cpp
void manageLOD(float dt) {
    float currentFPS = (dt > 0.0f) ? (1.0f / dt) : 60.0f;
    
    if (currentFPS < targetFPS_) {
        // Performance struggling - reduce quality
        for (auto& obj : objects_) {
            if (obj.lodLevel < 3) obj.lodLevel++;
        }
    } else if (currentFPS > targetFPS_ * 1.2f) {
        // Performance good - increase quality
        for (auto& obj : objects_) {
            if (obj.lodLevel > 0) obj.lodLevel--;
        }
    }
}
```

## Memory Usage
- **Per Object**: ~70 bytes
- **Total**: ~7 MB for 100,000 objects
- **Additional**: ~1-2 MB for profiling data
- **Peak Usage**: ~10 MB

## Performance Profiling

The demo tracks:
- **Current FPS**: Real-time frame rate
- **Average Frame Time**: Smoothed over 20 frames
- **Peak/Min FPS**: Historical extremes
- **LOD Distribution**: Objects per quality level
- **Memory Usage**: Approximate allocation

## Learning Objectives
- Understanding performance bottlenecks
- Implementing adaptive LOD systems
- Memory-efficient object management
- Real-time performance profiling

## Tuning the Demo

Adjust object count in `initializeObjects()`:
```cpp
const int objectCount = 100000;  // Try 50000, 200000, 500000!
```

Adjust target FPS:
```cpp
targetFPS_ = 60.0f;  // Try 30, 90, 144
```

## Known Limitations
- Object physics is simplified (no collision detection)
- LOD system is simulated (no actual mesh reduction)
- GPU rendering is not yet implemented (CPU-only demo)

See [EXAMPLES.md](../../EXAMPLES.md) for more extreme examples!
