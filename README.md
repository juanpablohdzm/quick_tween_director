# QuickTweenDirector

A visual timeline editor and runtime system for **QuickTween**. Create complex, reusable tween sequences directly inside your Actor Blueprints — no separate asset browser required.

> **Requires:** QuickTween plugin (must be enabled alongside this one).

---

## Table of Contents

1. [Installation](#installation)
2. [Opening the Director panel](#opening-the-director-panel)
3. [Creating an animation](#creating-an-animation)
4. [Using the Timeline Editor](#using-the-timeline-editor)
   - [Adding a track](#adding-a-track)
   - [Adding a step](#adding-a-step)
   - [Editing step parameters](#editing-step-parameters)
   - [Moving and deleting steps](#moving-and-deleting-steps)
   - [Zoom and navigation](#zoom-and-navigation)
   - [Saving](#saving)
5. [Playing the animation at runtime](#playing-the-animation-at-runtime)
6. [Slot bindings](#slot-bindings)
7. [Supported step types](#supported-step-types)
8. [Combining with other sequences](#combining-with-other-sequences)
9. [Playback control reference](#playback-control-reference)

---

## Installation

1. Copy the `QuickTweenDirector` folder into your project's `Plugins/` directory (next to the `QuickTween` folder).
2. Open your project. Unreal will prompt you to compile the new plugin — click **Yes**.
3. Go to **Edit → Plugins**, search for **QuickTweenDirector**, and make sure it is enabled.
4. Restart the editor if prompted.

```
Plugins/
  QuickTween/
  QuickTweenDirector/
```

---

## Opening the Director panel

The Director editor lives **inside the Blueprint editor** — it is not a standalone Content Browser tool.

1. Open any **Actor Blueprint** (double-click it in the Content Browser).
2. In the Blueprint editor toolbar, click the **Director** button (sequencer icon).
3. The **QuickTween Director** panel opens as a dockable tab. You can drag it to any position — dock it below the viewport or beside the event graph, just like UMG's Animations panel.

> The panel automatically shows the last-focused Actor Blueprint. If you have multiple Blueprint editors open, click **Director** from the one you want to work with to switch the panel's context.

---

## Creating an animation

With the Director panel open and an Actor Blueprint focused:

1. Click **+ New Animation** in the left column.
2. An animation named `NewAnimation` is created automatically.
   - A `UQuickTweenDirectorAsset` file is saved alongside your Blueprint in the Content folder.
   - A Blueprint variable of the same name is added to the Blueprint, pre-set to point at the asset.
3. **Rename** the animation by double-clicking its name in the list.
4. **Delete** an animation with the **✕** button next to its name (the asset file is kept; only the variable is removed).

You can have as many animations per Blueprint as you need — they all appear in the left column.

---

## Using the Timeline Editor

Selecting an animation in the left column opens its timeline on the right:

```
┌─ Toolbar ───────────────────────────────────────────────────────────┐
│  [Save]  [+ Add Track]              Duration: 2.50s   Zoom: ───○──  │
├─ Ruler ─────────────────────────────────────────────────────────────┤
│         0s      0.5s     1.0s     1.5s     2.0s     2.5s           │
├─ [StaticMeshComponent] ┬─ Step boxes ──────────────────────────────┤
│  UStaticMeshComponent  │  ████████ Relative Location ████           │
├─ [PointLightComponent] ┼───────────────────────────────────────────┤
│  UPointLightComponent  │         ████ Material Float ████           │
└────────────────────────┴───────────────────────────────────────────┘
```

### Adding a track

Each **track** targets one component from your Blueprint's component hierarchy.

1. Click **+ Add Track** in the toolbar.
2. The **Select Component** dialog opens, showing the full component tree of your Blueprint (mirrors the Components panel).
3. Pick a component and click **OK**. A track is added, labeled with the component's variable name and class.

> Only components from your Blueprint's Simple Construction Script are listed. Inherited C++ components may not appear — animate those via a custom slot binding.

### Adding a step

A **step** is a colored block on a track that represents one tween.

1. **Right-click** on an empty area of a track.
2. A **type submenu** appears, filtered to what makes sense for that component:

| Component type | Available types |
|----------------|----------------|
| Any `USceneComponent` | Relative Location, World Location, Relative Rotation, World Rotation, Relative Scale, World Scale |
| Any `UPrimitiveComponent` | All of the above + Material Scalar, Material Color |
| All | Delay (Empty) |

3. Select a type. The **Edit Step** dialog opens immediately so you can set the **From** / **To** values, duration, and easing.
4. Click **OK** — the step appears on the track in the matching color.

### Editing step parameters

Double-click any step to re-open its edit dialog.

**Common fields**

| Field | Description |
|-------|-------------|
| **Label** | Display name shown inside the step box |
| **Start Time** | Absolute start in seconds from the animation beginning |
| **Duration** | Length of one loop in seconds |
| **Time Scale** | Speed multiplier (2.0 = twice as fast) |
| **Loops** | Number of repeats. Use `-1` for infinite |

**Vector / Rotator steps**

| Field | Description |
|-------|-------------|
| **From** | Start value. Hidden when **From Current** is checked |
| **To** | End value |
| **From Current** | Reads the component's live value at start time instead of a fixed **From** |

**Float steps** — set **From**, **To**, and a **Parameter Name** (material scalar parameter name).

**Color steps** — click the swatches to open a color picker for **From** and **To**. Set **Parameter Name** for the material vector parameter name.

### Moving and deleting steps

| Action | How |
|--------|-----|
| **Move** | Left-click and drag horizontally. Snaps to 0.05 s grid. |
| **Delete** | Right-click the step → **Delete Step** |
| **Edit** | Double-click the step |

Steps on different tracks **overlap freely** and play in parallel at runtime.

### Zoom and navigation

- Use the **Zoom slider** in the toolbar (range 20–400 px/s).
- Scroll the track area vertically with the mouse wheel.

### Saving

Click **Save** in the toolbar. The Director asset is an ordinary `UDataAsset` saved to your Content folder. The Blueprint variable pointing to it is saved when you save the Blueprint (Ctrl+S in the Blueprint editor).

---

## Playing the animation at runtime

Because the Director panel added a **Blueprint variable** for each animation, using it at runtime is straightforward.

**Simplest case — actor animates its own components:**

```
[Event BeginPlay]
    │
    ▼
[Create Director Player]
    Asset ← MyAnimation  (drag the variable from My Blueprint panel)
    World Context Object ← self
    │
    ▼
[Play]
```

That's it. Components are **automatically bound** — no manual `Bind Slot` calls needed when the animation was created for this Blueprint's components.

**Chaining events:**

```
[Create Director Player] ──► [Assign On Complete ← MyCallback] ──► [Play]
```

Or bind directly using the **BlueprintAssignable** delegates on the returned player:

```
[Create Director Player] → return value → [OnComplete (Assign Event)] → [Play]
```

**Accessing individual step tweens for callbacks:**

```
[Create Director Player] ──► [Play]
                         ──► [Get Tween By Label "MyStep"] ──► [Assign On Complete Event]
```

**Storing the player for later control:**

```
[Create Director Player] ──► [SET MyAnimationPlayer (variable)]
                         ──► [Play]

[Event SomeTrigger]      ──► [GET MyAnimationPlayer] ──► [Pause]
```

---

## Slot bindings

Each track stores the **variable name** of the component it was created for. At runtime, `Build()` walks the owning actor's component list and binds each slot automatically.

| Situation | What happens |
|-----------|-------------|
| Animation created for this Blueprint's component | Auto-bound — no action required |
| You call `Bind Slot(Name, Object)` before `Play()` | Manual binding overrides the automatic one |
| Slot name is empty | Falls back to the world context object (actor root component) |
| Component not found and no manual binding | Step is skipped with a log warning |

For **material parameter** steps (Float → Material Scalar, Color → Material Vector) the slot must be a `UMaterialInstanceDynamic`. Bind it manually since dynamic material instances are created at runtime:

```
[Create and Set Material Instance Dynamic] ──► [Bind Slot ("MyMeshSlot", MID)]
[Create Director Player] ──► [Bind Slot ("MyMeshSlot", MID)] ──► [Play]
```

---

## Supported step types

### Vector
Animates an `FVector` property on a `USceneComponent`.

| Sub-type | Property |
|----------|----------|
| Relative Location | `SetRelativeLocation` |
| World Location | `SetWorldLocation` |
| Relative Scale | `SetRelativeScale3D` |
| World Scale | `SetWorldScale3D` |

### Rotator
Animates an `FRotator`, always using the shortest angular path.

| Sub-type | Property |
|----------|----------|
| Relative Rotation | `SetRelativeRotation` |
| World Rotation | `SetWorldRotation` |

### Float (Material Scalar)
Sets a scalar parameter on a `UMaterialInstanceDynamic`. Requires **Parameter Name** and a manually bound MID slot.

### Linear Color (Material Vector)
Sets a vector parameter on a `UMaterialInstanceDynamic` (RGB only). Requires **Parameter Name** and a manually bound MID slot.

### Empty (Delay)
A no-op step. Use it to add pauses within a track without affecting parallel tracks.

---

## Combining with other sequences

`UQuickTweenDirectorPlayer` implements `UQuickTweenable`, so it can be nested inside any QuickTween sequence using the standard **Quick Tween Create Sequence → Append / Join** nodes, passing the Director Player variable as the `Tween` argument.

---

## Playback control reference

| Node | Description |
|--------|-------------|
| `Play` | Start or resume. Calls `Build` automatically on first play. |
| `Pause` | Pause at the current position. |
| `Reverse` | Toggle playback direction. |
| `Restart` | Reset to Idle. Call `Play` again to restart. |
| `Complete` | Jump to the end state immediately. |
| `Kill` | Stop and unregister. Do not use the player after this. |
| `Get Tween By Step Id` | Returns the tween for a specific step ID (must be built first). |
| `Get Tween By Label` | Returns the first tween whose label matches (must be built first). |

**Events** (drag off the player reference and use the `OnXxx` delegate pins, or use `Assign Event` nodes):

| Event | Fires when |
|-------|-----------|
| `OnStart` | First `Play` exits Idle |
| `OnUpdate` | Every tick while playing |
| `OnLoop` | Each time the animation loops |
| `OnComplete` | The last loop finishes |
| `OnKilled` | `Kill` is called |

**State queries:**

| Node | Returns |
|----------|---------|
| `Get Is Playing` | `true` while ticking |
| `Get Is Completed` | `true` after the last loop |
| `Get Is Pending Kill` | `true` after `Kill` |
| `Get Elapsed Time` | Seconds since playback started |
| `Get Loop Duration` | Duration of one loop |
| `Get Total Duration` | `Loop Duration × Loops` |
| `Get Current Loop` | Current loop index (0-based) |
