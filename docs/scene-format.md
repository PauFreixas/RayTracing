# Scene format (`.sdf`)

A scene is a plain-text file read line by line by `Reader::ReadSceneDescription` ([src/Reader.cpp](../src/Reader.cpp)).
Each line holds exactly one entry. The default scene is [assets/scene.sdf](../assets/scene.sdf).

## General rules

- **Comments** start with `#` in the **first column**. An indented `#` is not a comment and causes a parse error.
- **Blank lines** (spaces and tabs only) are ignored.
- **Vectors** (points and directions) use parentheses: `(x, y, z)`.
- **Colours** use square brackets, with each channel normally in `[0, 1]`: `[r, g, b]`.
- **Scalars** are plain numbers: `3.1`.
- Write the keyword, then whitespace, then the value. Spaces after commas are fine. A space **before** a comma is not.
- Reading stops at the first line that isn't recognised, and the error is printed as `Error reading scene file, line N: ...`.
  The line number counts non-blank, non-comment lines only.

## Coordinate system

Right-handed, with Y up:

```
  Y
  |
  |
   ------ X
   \
    \
     Z   (towards the viewer in the default scene)
```

## Camera

| Keyword  | Value   | Meaning |
|----------|---------|---------|
| `eye`    | `(x,y,z)` | Camera position. |
| `lookat` | `(x,y,z)` | Point the camera looks at. |
| `up`     | `(x,y,z)` | Up direction. Must not be parallel to the gaze direction. |
| `vpdist` | number  | Distance from the eye to the view plane. Larger values give a narrower field of view. |

## Lights and global settings

| Keyword      | Value     | Meaning |
|--------------|-----------|---------|
| `lightpos`   | `(x,y,z)` | Position of a point light. |
| `lightcolor` | `[r,g,b]` | Colour of that light. **Must come after its `lightpos`**: reading `lightcolor` is what completes the light. |
| `amblight`   | `[r,g,b]` | Ambient light. A single global value; the last one in the file wins. |
| `bgcolor`    | `[r,g,b]` | Colour of rays that hit nothing. A single global value; the last one in the file wins. |

At most **10** lights are supported, and this limit is not checked.

## Objects

| Keyword    | Syntax | Notes |
|------------|--------|-------|
| `sphere`   | `sphere (cx,cy,cz) radius` | |
| `cube`     | `cube (x1,y1,z1) (x2,y2,z2)` | Axis-aligned box given by two opposite corners. |
| `triangle` | `triangle (ax,ay,az) (bx,by,bz) (cx,cy,cz)` | |
| `polygon`  | `polygon (a) (b) (c) (d) (e)` | Five vertices, each `(x,y,z)`. Parsed, but its intersection is not implemented yet, so it is invisible. |

## Materials

Material lines apply to the **most recently declared object**, so they must follow an object line.
A material line before any object crashes the reader.

| Keyword            | Value     | Meaning | Used by the renderer today? |
|--------------------|-----------|---------|-----------------------------|
| `diffuse`          | `[r,g,b]` | Diffuse (Lambertian) colour. | Yes |
| `specular`         | `[r,g,b]` | Colour of specular highlights. | Not yet (M1) |
| `Phong_exp`        | number    | Phong exponent. Higher values give tighter highlights. | Not yet (M1) |
| `reflectivity`     | number in `[0,1]` | Weight of mirror reflection. | Not yet (M1) |
| `refractive_index` | number    | Index of refraction (speed of light in a vacuum / in the material). | Not yet (M1) |
| `opacity`          | number in `[0,1]` | 0 = fully transparent, 1 = opaque. | Not yet (M1) |
| `emission`         | `[r,g,b]` | Emitted light. Any non-zero value makes the object an emitter. | Only to skip emitters in `Trace` |

Set every material field you rely on: fields you leave out are not guaranteed to start at zero.

## Minimal example

```
# Camera
eye     (0, 0, 10)
lookat  (0, 0, 0)
up      (0, 1, 0)
vpdist  2.0

# One white light
lightpos   (5, 5, 5)
lightcolor [1, 1, 1]
amblight   [0.1, 0.1, 0.1]
bgcolor    [0.05, 0.05, 0.1]

# A red sphere
sphere   (0, 0, 0) 1.5
diffuse  [0.9, 0.2, 0.2]
specular [1, 1, 1]
Phong_exp 20
reflectivity 0
opacity  1
emission [0, 0, 0]
```
