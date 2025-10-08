# Animated Scene Demo

## Overview
This example demonstrates the creative power of procedural animation in Aurora3D.

## Features
- **Multiple Animation Systems**: Waves, spirals, orbits, pulses, Fibonacci patterns
- **1000+ Animated Objects**: Each with independent animation state
- **Procedural Particle Systems**: Real-time particle emission and simulation
- **Mathematical Beauty**: Complex transformations bring math to life
- **Color Cycling**: Dynamic color effects based on time

## What Makes It Extreme?
- **1000+ individually animated objects**
- **Zero loading time** - all content generated procedurally
- **Complex math transformations** in real-time
- **Smooth 144+ FPS** animation
- **5 concurrent animation systems**

## Running the Example
```bash
./build/bin/Debug/animated_scene
# or on Linux
./build/bin/animated_scene
```

## Sample Output
```
=== Animated Scene Demo ===
Initializing procedural animation system...
✓ Animation systems: 5
✓ Animated objects: 1000
✓ Particle emitters: 3
✓ Target FPS: 144+

Watch as mathematics comes to life!

┌────────────────────────────────────────────┐
│ ANIMATION STATUS @ 5.23s
├────────────────────────────────────────────┤
│ Wave System: 200 objects @ 2Hz
│ Spiral Galaxy: 300 objects @ 0.5Hz
│ Orbital Dance: 250 objects @ 1Hz
│ Pulse Field: 150 objects @ 3Hz
│ Fibonacci Bloom: 100 objects @ 1.5Hz
├────────────────────────────────────────────┤
│ Total Objects: 1000
│ Active Particles: 287
│ Color Hue: 306.9°
│ Frames: 753
└────────────────────────────────────────────┘
```

## Animation Systems

### 1. Wave System (200 objects)
Creates a rippling wave effect through 3D space:
```cpp
obj.currentY = obj.baseY + 
    std::sin(t * frequency + obj.baseX * 0.1f) * amplitude;
obj.rotation = std::sin(t) * PI;
```
- **Frequency**: 2 Hz
- **Amplitude**: 5 units
- **Effect**: Smooth sine wave motion

### 2. Spiral Galaxy (300 objects)
Objects rotate in a spiral pattern:
```cpp
float spiralAngle = t * frequency;
float spiralRadius = sqrt(x² + z²);
obj.currentX = cos(spiralAngle) * spiralRadius;
obj.currentZ = sin(spiralAngle) * spiralRadius;
```
- **Frequency**: 0.5 Hz (slow rotation)
- **Amplitude**: 10 units vertical bob
- **Effect**: Galaxy-like spiral motion

### 3. Orbital Dance (250 objects)
Objects orbit around their base positions:
```cpp
obj.currentX = baseX + cos(orbitAngle) * amplitude;
obj.currentZ = baseZ + sin(orbitAngle) * amplitude;
```
- **Frequency**: 1 Hz
- **Amplitude**: 8 units orbit radius
- **Effect**: Circular orbital motion

### 4. Pulse Field (150 objects)
Objects pulse in size based on sine wave:
```cpp
float pulse = abs(sin(t * frequency));
obj.scale = 0.5f + pulse * 1.5f;  // Range: 0.5 to 2.0
```
- **Frequency**: 3 Hz (fast pulsing)
- **Scale Range**: 0.5x to 2.0x
- **Effect**: Rhythmic pulsing

### 5. Fibonacci Bloom (100 objects)
Objects follow the golden ratio spiral:
```cpp
float phi = 1.618033988749895f;  // Golden ratio
float fibAngle = localTime * phi * frequency;
float fibRadius = sqrt(localTime) * amplitude;
obj.currentX = cos(fibAngle) * fibRadius;
obj.currentZ = sin(fibAngle) * fibRadius;
```
- **Frequency**: 1.5 Hz
- **Pattern**: Golden ratio (φ = 1.618...)
- **Effect**: Natural spiral growth pattern

## Particle Systems

### Particle Emitters
Three emitters create dynamic particle fields:

| Emitter | Position | Rate | Lifetime | Behavior |
|---------|----------|------|----------|----------|
| Center | (0, 10, 0) | 100/s | 2.0s | Fountain |
| East | (15, 5, 15) | 150/s | 1.5s | Burst |
| West | (-15, 5, -15) | 150/s | 1.5s | Burst |

**Total Particle Count**: ~400-500 active particles at any time

## Color Cycling
```cpp
currentHue_ = fmod(globalTime * 0.2f, 1.0f);
```
- **Cycle Speed**: 5 seconds per full rotation
- **Color Space**: HSV with full hue rotation
- **Range**: 0° to 360°

## Technical Details

### Performance Characteristics
- **Object Update Cost**: O(n) where n = 1000
- **Math Operations**: ~5000 sin/cos per frame
- **Memory Usage**: ~100 KB for object data
- **Target FPS**: 144+

### Animation Timing
- **Delta-time based**: Smooth regardless of frame rate
- **Phase staggering**: Objects start at different phases
- **No discontinuities**: Smooth, continuous motion

## Code Highlights

### Procedural Object Creation
```cpp
for (int i = 0; i < system.objectCount; ++i) {
    AnimatedObject obj;
    float angle = (i / objectCount) * 2π * 3.0f;
    float radius = 20.0f + systemId * 5.0f;
    
    obj.baseX = cos(angle) * radius;
    obj.baseZ = sin(angle) * radius;
    obj.localTime = i * 0.01f;  // Stagger animations
    
    animatedObjects_.push_back(obj);
}
```

### Time-based Animation Update
```cpp
void updateProceduralObjects(float dt) {
    for (auto& obj : animatedObjects_) {
        const auto& system = animationSystems_[obj.systemId];
        obj.localTime += dt;
        
        float t = obj.localTime + system.phase;
        // Apply system-specific transformation
        applyAnimation(obj, system, t);
    }
}
```

## Learning Objectives
- Procedural content generation
- Time-based animation systems
- Mathematical transformations (trig, spirals)
- Particle system basics
- Performance optimization for math-heavy code

## Customization Ideas

### Add New Animation System
```cpp
else if (system.type == "custom") {
    // Your custom math here!
    obj.currentX = baseX * cos(t);
    obj.currentY = baseY * sin(t * 2);
    obj.currentZ = baseZ * tan(t * 0.5);
}
```

### Increase Object Count
```cpp
{"Wave System", "wave", 2.0f, 5.0f, 0.0f, 500},  // 200 -> 500
```

### Change Animation Speed
```cpp
{"Spiral Galaxy", "spiral", 2.0f, 10.0f, 0.0f, 300},  // 0.5Hz -> 2Hz
```

## Mathematical Background

### Sine Waves
Used for smooth oscillation: `y = A·sin(ωt + φ)`
- A = amplitude
- ω = angular frequency
- φ = phase

### Parametric Equations
Used for spirals: `x = r·cos(θ), y = r·sin(θ)`

### Golden Ratio (φ)
Used in Fibonacci: `φ = (1 + √5) / 2 ≈ 1.618`

## Visual Effects Achieved
- ✨ Flowing wave motions
- 🌌 Rotating galaxy patterns
- 🔄 Orbital dance choreography
- 💓 Rhythmic pulsing
- 🌸 Natural growth patterns (Fibonacci)
- 🎨 Smooth color transitions

See [EXAMPLES.md](../../EXAMPLES.md) for more extreme examples!
