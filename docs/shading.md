# How shading works

The renderer is a classic **Whitted ray tracer**. For every pixel, a ray leaves the camera and finds the closest surface it hits (`Raytracer::Cast`). `Raytracer::Shade` then works out the colour of that point, sending more rays for shadows, reflection and refraction.
The code is in [src/Raytracer.cpp](../src/Raytracer.cpp).

## Notation

| Symbol | Meaning |
|---|---|
| `P` | The point that was hit |
| `N` | Unit surface normal, flipped if needed so that it faces the incoming ray |
| `V` | Unit vector from `P` towards the viewer (the ray's origin) |
| `L` | Unit vector from `P` towards a light |
| `R` | `L` mirrored about `N`: `R = 2 (N·L) N − L` |

## The shading equation

```
colour = emission
       + kd * diffuse * ambient
       + Σ over lights not in shadow:  kd * diffuse * lightColour * (N·L)          (Lambert)
                                     + specular * lightColour * max(0, R·V)^n     (Phong)
       + kr * Trace(reflected ray)
       + kt * Trace(refracted ray)
```

The weights `kd`, `kr` and `kt` split the light at the surface between diffuse, mirror and glass behaviour. They always add up to 1:

| Weight | Formula | Meaning |
|---|---|---|
| `kd` (diffuse) | `opacity · (1 − reflectivity)` | Matte part of an opaque surface |
| `kr` (reflection) | `opacity · reflectivity + (1 − opacity) · F` | Mirror part, plus the light glass reflects |
| `kt` (transmission) | `(1 − opacity) · (1 − F)` | Light that passes through glass |

`F` is the Fresnel reflectance, described below. Phong highlights are not weighted, because the `specular` colour already sets their strength.

## Hard shadows

For each light, a **shadow ray** is cast from `P` towards the light. If it hits any object closer than the light, that light is skipped. Two lights of different colours therefore give coloured shadows: a point that only one light can see takes that light's colour.

Every object blocks light fully, including glass. That's the usual Whitted simplification; real glass would let some light through and focus it into caustics.

### Shadow acne

Because of floating-point rounding, a ray that starts exactly on a surface can hit that same surface again at a tiny distance, which speckles the image with dark dots. To avoid this, every secondary ray starts slightly away from the surface: `P + ε·N` for shadow and reflection rays, and `P − ε·N` for refracted rays, which continue into the object. `ε` is `RayBias = 0.001`, which is small compared with the scene's size (the Cornell box is 20 units wide).

## Mirror reflection

The reflected direction is `D − 2 (D·N) N`, where `D` is the incoming ray's direction. `Trace` follows it recursively. Recursion stops after `tree_depth = 5` bounces, so two facing mirrors can't loop forever.

## Refraction

When `opacity < 1`, the ray also continues **through** the surface, bent by **Snell's law**:

```
n1 · sin(θ1) = n2 · sin(θ2)
```

`n1` and `n2` are the refractive indices on each side. A ray entering the object goes from air (`n1 = 1`) into the material (`n2 = refractive_index`). A ray leaving it goes the other way. The code tells the two apart from whether the normal faced the incoming ray before it was flipped.

### Total internal reflection

When light goes from a denser to a less dense material (for example leaving glass) at a shallow enough angle, Snell's law has no solution (`sin θ2 > 1`). All of the light is then reflected. `Refraction()` in [src/Vec3.h](../src/Vec3.h) returns a zero vector in this case, and the shader sets `F = 1`.

### Fresnel reflection (Schlick's approximation)

Glass reflects more light at grazing angles than when you look straight at it. This is why the glass sphere's edges show the walls. The exact Fresnel equations are costly, so the renderer uses Schlick's approximation:

```
F0 = ((n1 − n2) / (n1 + n2))²
F  = F0 + (1 − F0) · (1 − cos θ)^5
```

`θ` is the angle on the less dense side of the surface: the incoming angle when entering, the outgoing angle when leaving. For glass (`n = 1.5`), `F0 = 0.04`: about 4% of light is reflected head-on, and close to 100% at grazing angles.

## Ray–triangle and ray–polygon intersection

- **Triangles** use the **Möller–Trumbore** algorithm. It solves `origin + s·direction = A + u·(B − A) + v·(C − A)` for the distance `s` and the barycentric coordinates `u` and `v`. The point is inside the triangle when `u ≥ 0`, `v ≥ 0` and `u + v ≤ 1`.
- **Polygons** (five corners, flat and convex) intersect the ray with the polygon's plane. The hit point is inside when it lies on the same side of every edge.

## Sources

- T. Whitted, "An Improved Illumination Model for Shaded Display", *Communications of the ACM* 23(6), 1980.
- B. T. Phong, "Illumination for Computer Generated Pictures", *Communications of the ACM* 18(6), 1975.
- C. Schlick, "An Inexpensive BRDF Model for Physically-based Rendering", *Computer Graphics Forum* 13(3), 1994.
- T. Möller and B. Trumbore, "Fast, Minimum Storage Ray/Triangle Intersection", *Journal of Graphics Tools* 2(1), 1997.
- M. Pharr, W. Jakob and G. Humphreys, *Physically Based Rendering*, 4th edition, 2023, chapters on reflection models and specular transmission (free online at pbr-book.org).
