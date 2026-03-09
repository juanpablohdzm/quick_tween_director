# QuickTweenDirector

**QuickTweenDirector** is a visual timeline editor and runtime playback system for the **QuickTween** plugin. It lets you author multi-track, multi-step tween animations directly inside any Actor Blueprint and play them back at runtime with a single Blueprint node — no code required.

Think of it as a lightweight in-editor Sequencer that lives inside your Blueprint editor, produces a reusable data asset, and plays back through the QuickTween engine.

> **Requires:** The **QuickTween** plugin must be installed and enabled alongside this one.

---

## Table of Contents

1. [What it does](#what-it-does)
2. [Installation](#installation)
3. [Opening the Director panel](#opening-the-director-panel)
4. [Creating an animation asset](#creating-an-animation-asset)
5. [Timeline editor](#timeline-editor)
   - [Toolbar reference](#toolbar-reference)
   - [Adding and reordering tracks](#adding-and-reordering-tracks)
   - [Adding steps](#adding-steps)
   - [Editing step values](#editing-step-values)
   - [Moving steps](#moving-steps)
   - [Resizing steps](#resizing-steps)
   - [Multi-selecting steps](#multi-selecting-steps)
   - [Copying and pasting steps](#copying-and-pasting-steps)
   - [Deleting steps and tracks](#deleting-steps-and-tracks)
   - [Grid snapping](#grid-snapping)
   - [In-editor playback preview](#in-editor-playback-preview)
   - [Zoom and fit](#zoom-and-fit)
   - [Exporting to JSON](#exporting-to-json)
   - [Saving](#saving)
6. [Playing the animation at runtime](#playing-the-animation-at-runtime)
7. [Slot bindings](#slot-bindings)
8. [Debugging binding errors](#debugging-binding-errors)
9. [Blueprint events](#blueprint-events)
10. [Supported step types](#supported-step-types)
11. [Step dialog field reference](#step-dialog-field-reference)
12. [Playback API reference](#playback-api-reference)
13. [Combining with other sequences](#combining-with-other-sequences)

---

## What it does

QuickTweenDirector solves the problem of creating complex, multi-component animations that need to be reusable and data-driven. Without it, you would write many BeginPlay tweens manually, hard-code timings, and duplicate logic across Blueprints. With it:

- **Design animations visually** on a timeline — place, move, and resize colored step blocks per component track.
- **Store animations as assets** (`UQuickTweenDirectorAsset`) that live in your Content folder, serialise with your project, and can be referenced from multiple Blueprints.
- **Play at runtime** with `Create Director Player → Play`. Components are auto-bound; no `Bind Slot` calls needed for Blueprint components.
- **React to events** in Blueprint: `OnStart`, `OnComplete`, `OnLoop`, per-step `OnStepBegin` / `OnStepEnd`.
- **Undo/Redo** every edit — all track and step changes are fully transactional.

---

## Installation

1. Copy the `QuickTweenDirector` folder into your project's `Plugins/` directory alongside the `QuickTween` folder:

```
YourProject/
  Plugins/
    QuickTween/
    QuickTweenDirector/
```

2. Right-click your `.uproject` file → **Generate Visual Studio project files** (or your IDE equivalent).
3. Open your project. Unreal will prompt you to compile the new plugin — click **Yes**.
4. Go to **Edit → Plugins**, search for **QuickTweenDirector**, and verify it is enabled.
5. Restart the editor if prompted.

---

## Opening the Director panel

The Director editor is embedded inside the **Blueprint editor** — it is not a standalone Content Browser window.

1. Open any **Actor Blueprint** (double-click it in the Content Browser).
2. In the Blueprint editor toolbar, click the **Director** button.
3. The **QuickTween Director** panel opens as a dockable tab. Drag it wherever you like — below the viewport, beside the event graph, or as a floating window.

> The panel tracks the last-focused Actor Blueprint automatically. If you have multiple Blueprint editors open, clicking **Director** from the correct one switches the panel's context.

---

## Creating an animation asset

With the Director panel open and an Actor Blueprint focused:

1. Click **+ New Animation** in the left column.
2. A `UQuickTweenDirectorAsset` file is created alongside your Blueprint in the Content folder, and a Blueprint variable is added automatically (pre-set to point at the new asset).
3. **Rename** the animation by double-clicking its entry in the list.
4. **Delete** the entry with the **✕** button — the asset file is kept on disk; only the variable reference is removed.

You can have as many animations per Blueprint as you need. All appear in the left column.

---

## Timeline editor

Clicking an animation in the left column opens its timeline:

```
┌─ Toolbar ─────────────────────────────────────────────────────────────────────┐
│ [Save] [+ Add Track] | [▶][⏸][⏹] 0.00s | Duration: 4.50s | [Snap] [Fit] ──○── │
├────────────────────────┬──────────────────────────────────────────────────────┤
│ TIMELINE               │  0s      1s      2s      3s      4s                  │
├─ StaticMesh            │  ████████ Relative Location ████│                    │
│   StaticMeshComponent  │                                                       │
├─ Light                 │          ████ Material Float ████████                 │
│   PointLightComponent  │                                                       │
└────────────────────────┴──────────────────────────────────────────────────────┘
```

### Toolbar reference

| Button / Control | What it does |
|-----------------|--------------|
| **Save** | Saves the director asset to disk. |
| **+ Add Track** | Opens the component picker dialog and adds a new track. |
| **▶ Play** | Starts the in-editor visual playback preview from the current playhead position. Turns green while active. |
| **⏸ Pause** | Pauses the in-editor preview at the current playhead position. |
| **⏹ Stop** | Stops and resets the playhead to 0:00. |
| **Playhead time** | Read-only display of the current playhead position in seconds. |
| **Duration badge** | Shows the total animation duration (max step end time across all tracks). |
| **Snap** | Toggle checkbox. When enabled, drag and resize operations snap to 0.05 s increments. Highlighted orange when active. |
| **Fit** | Sets the zoom level so the entire animation fits in the visible timeline area. |
| **Zoom slider** | Adjusts pixels per second (20–400 px/s). The percentage shown is relative to the 80 px/s default. |
| **Export JSON** | Opens a save dialog and writes all tracks and steps to a `.json` file. |

---

### Adding and reordering tracks

Each **track** targets one component from your Blueprint's component tree.

**To add a track:**
1. Click **+ Add Track** in the toolbar.
2. The **Select Component** dialog shows the full SCS component hierarchy of your Blueprint.
3. Select a component and click **OK**.

**To reorder tracks:**
- Use the **▲ / ▼** arrow buttons on the left edge of each track label to move it up or down one position. The change is undoable.

> Only Blueprint SCS components appear in the picker. Inherited C++ components can be animated via manual `Bind Slot` calls at runtime.

---

### Adding steps

A **step** is a colored block on a track that represents one tween animation.

1. **Right-click** on an empty area of a track row.
2. The **Add Step** submenu appears, filtered to what makes sense for the track's component class:

| Component type | Available step types |
|----------------|---------------------|
| Any `USceneComponent` | Relative Location, World Location, Relative Rotation, World Rotation, Relative Scale, World Scale |
| Any `UPrimitiveComponent` | All of the above + Material Scalar Parameter, Material Vector Parameter (Color) |
| All tracks | Delay (Empty) |

3. Select a type. The **Edit Step** dialog opens so you can set From/To values and timing.
4. Click **Apply**. The step appears on the track as a colored block.

Steps on different tracks overlap freely and play in parallel at runtime. New steps are appended after the last existing step on the same track by default.

---

### Editing step values

**Double-click** any step block to re-open the Edit Step dialog.

See [Step dialog field reference](#step-dialog-field-reference) for a description of every field.

---

### Moving steps

- **Left-click and drag** a step block horizontally to change its start time.
- The step preview updates live during the drag.
- With **Snap** enabled, the start time snaps to 0.05 s increments.
- Release the mouse to commit. The change is undoable.

---

### Resizing steps

- Hover over the **right edge** of any step block — the cursor changes to a horizontal resize arrow.
- **Left-click and drag** left or right to change the step's duration.
- The duration updates live. With **Snap** enabled, it snaps to 0.05 s increments.
- Release to commit. The change is undoable.

---

### Multi-selecting steps

- **Ctrl + left-click** steps to toggle them in/out of the selection. Selected steps show a white outline.
- Click on empty track space (without Ctrl) to clear the selection.
- Right-clicking with multiple steps selected adds a **Delete Selected** option to the context menu.

---

### Copying and pasting steps

- **Right-click** a step → **Copy Step** — copies it to a module-level clipboard.
- **Right-click** on empty space in any track → **Paste Step** — pastes a copy of the clipboard step at the click position (on the target track), with a new ID. The pasted step inherits all fields (type, duration, easing, values) from the original but is bound to the new track's slot.

---

### Deleting steps and tracks

| What | How |
|------|-----|
| Delete a single step | Right-click the step → **Delete Step** |
| Delete all selected steps | Right-click with multiple steps selected → **Delete Selected** |
| Delete a track (and all its steps) | Click the **✕** button on the track label |

All deletions are undoable.

---

### Grid snapping

Click the **Snap** toggle in the toolbar to enable grid snapping. When active:

- **Drag-move** snaps step start times to the nearest 0.05 s boundary.
- **Resize** snaps step durations to 0.05 s boundaries.
- **New step placement** (via right-click → Add Step) snaps the start time.

The snap increment is fixed at **0.05 seconds** and is indicated in the toolbar when Snap is on (text turns orange).

---

### In-editor playback preview

The toolbar's Play/Pause/Stop controls drive a **visual playhead** — a green vertical line on the ruler that advances in real time.

- **▶ Play**: starts advancing the playhead from its current position. The play button glows green while active. Automatically stops when the playhead reaches the end of the animation.
- **⏸ Pause**: freezes the playhead where it is.
- **⏹ Stop**: resets the playhead to 0:00.
- **Click or drag on the ruler** to scrub the playhead to any position manually.

> This is a **visual-only preview** — it does not instantiate tweens or move actors. It lets you verify timings and read the step colors/labels against the ruler. Runtime playback (which actually moves your actors) happens via `Create Director Player → Play` in-game.

Easing curves are also previewed visually inside step blocks when the block is wide enough: a subtle curve line shows the shape of the chosen easing function.

---

### Zoom and fit

- **Zoom slider**: drag right to zoom in (more detail), left to zoom out. Shows the zoom level as a percentage of the 80 px/s default.
- **Fit**: automatically adjusts the zoom so the full animation duration fits in the current visible timeline area. Useful after adding many long steps.

---

### Exporting to JSON

Click **Export JSON** in the toolbar to serialize the entire asset to a `.json` file.

The dialog lets you choose a file path. The output contains:
- Asset name and total duration
- All tracks (ID, label, component variable name)
- All steps (ID, track ID, label, type, start time, duration, loops, easing, slot name)

This is useful for version-diffing, external tooling, or data inspection. The JSON file is not used at runtime.

---

### Saving

Click **Save** in the toolbar to save the Director asset. The asset is an ordinary `UDataAsset` file in your Content folder. The Blueprint variable pointing to it is saved when you save the Blueprint (Ctrl+S in the Blueprint editor).

---

## Playing the animation at runtime

Because the Director panel added a **Blueprint variable** for each animation, using it at runtime requires just two nodes.

**Simplest case — actor animates its own Blueprint components:**

```
[Event BeginPlay]
    │
    ▼
[Create Director Player]  ← Asset: drag your animation variable here
    World Context Object  ← self
    │
    ▼
[Play]
```

Blueprint components are **automatically bound** by name. No `Bind Slot` calls are needed.

**Storing the player for later control:**

```
[Create Director Player] ──► [SET MyPlayer (variable)] ──► [Play]

[Event OnTrigger]         ──► [GET MyPlayer] ──► [Pause]
[Event OnRelease]         ──► [GET MyPlayer] ──► [Play]
```

**Reacting when it finishes:**

```
[Create Director Player]
    │
    ├──► [Assign OnComplete ← My Callback Event]
    │
    └──► [Play]
```

**Reacting to individual step boundaries:**

```
[Create Director Player]
    │
    ├──► [Assign OnStepBegin ← HandleStepBegin]
    ├──► [Assign OnStepEnd   ← HandleStepEnd]
    └──► [Play]

[HandleStepBegin]  ── receives SlotName (FName) + StepId (FGuid)
[HandleStepEnd]    ── same parameters
```

**Accessing a specific step's tween for callbacks:**

```
[Create Director Player] ──► [Play]
                         ──► [Get Tween By Label "MyStep"] ──► [Assign OnComplete Event]
```

---

## Slot bindings

At runtime, `Build()` (called automatically on first `Play`) maps each track's component variable name to the matching component on the owning actor.

| Situation | Result |
|-----------|--------|
| Animation created for this Blueprint's component | Automatically bound — no action needed |
| `Bind Slot(Name, Object)` called before `Play()` | Manual binding takes priority over the automatic one |
| Slot name is empty | Falls back to the world context object (actor's root component) |
| Slot bound to `UMaterialInstanceDynamic` | Used for Material Scalar / Material Vector steps |
| Component not found, no manual binding | Step is skipped; error logged to `BindingErrors` |

**Binding a Material Instance Dynamic (for material steps):**

Create your MID first, then bind it before calling `Play`:

```
[Create Dynamic Material Instance] → MID
[Create Director Player]           → Player
[Player → Bind Slot] Name="MyMesh"  Object=MID
[Player → Play]
```

---

## Debugging binding errors

After `Build()` runs, check `Player → Binding Errors` (a `TArray<FString>`, `BlueprintReadOnly`). If it is non-empty, each entry describes a slot that could not be auto-bound:

```
Track 'Cube': slot 'Cube' could not be auto-bound — no matching component or property found on 'BP_MyActor'.
```

Common causes:
- The component was renamed in the Blueprint after the track was created.
- The track was created from a different Blueprint and pasted into this one.
- The component is a C++ native component (not in the SCS tree).

Fix: call `Bind Slot` manually with the correct object before `Play`, or recreate the track from the correct component.

---

## Blueprint events

All events are `BlueprintAssignable` multicast delegates. Drag from the player reference and use **Assign Event** nodes, or use the `OnXxx` pins directly.

| Event | Signature | Fires when |
|-------|-----------|-----------|
| `OnStart` | `(Player)` | First `Play` from the Idle state |
| `OnUpdate` | `(Player)` | Every tick while playing |
| `OnLoop` | `(Player)` | Each time the animation completes one full loop |
| `OnComplete` | `(Player)` | The final loop finishes |
| `OnKilled` | `(Player)` | `Kill` is called |
| `OnStepBegin` | `(Player, SlotName, StepId)` | A step's window becomes active (local time enters `[StartTime, EndTime]`) |
| `OnStepEnd` | `(Player, SlotName, StepId)` | A step's window becomes inactive (local time leaves `[StartTime, EndTime]`) |

`OnStepBegin` and `OnStepEnd` fire once per step per loop iteration. They receive the step's **slot name** (the component variable name) and **step ID** (the `FGuid` you can match against `Get Tween By Step Id`).

**Read-only playback properties** (available as `BlueprintReadOnly`):

| Property | Type | Description |
|----------|------|-------------|
| `CurrentLocalTime` | `float` | Current position within the current loop, in seconds |
| `BindingErrors` | `TArray<FString>` | Populated after `Build()` if any slots failed to bind |

---

## Supported step types

### Vector
Animates an `FVector` property on a `USceneComponent`. Sub-types:

| Label | Drives |
|-------|--------|
| Relative Location | `SetRelativeLocation` |
| World Location | `SetWorldLocation` |
| Relative Scale | `SetRelativeScale3D` |
| World Scale | `SetWorldScale3D` |

Color in the timeline: **orange** (location) or **amber** (scale).

### Rotator
Animates an `FRotator` on a `USceneComponent`, always taking the shortest angular path. Sub-types:

| Label | Drives |
|-------|--------|
| Relative Rotation | `SetRelativeRotation` |
| World Rotation | `SetWorldRotation` |

Color in the timeline: **teal**.

### Float (Material Scalar Parameter)
Sets a scalar parameter on a `UMaterialInstanceDynamic`. Requires:
- **Parameter Name** — the exact name of the material scalar parameter.
- The slot must be manually bound to a `UMaterialInstanceDynamic` before `Play`.

Color in the timeline: **blue**.

### Linear Color (Material Vector Parameter)
Sets an RGB vector parameter on a `UMaterialInstanceDynamic`. Requires:
- **Parameter Name** — the material vector parameter name.
- The slot must be manually bound to a `UMaterialInstanceDynamic` before `Play`.

Color in the timeline: **purple**.

### Empty (Delay)
A no-op step that consumes time on its track without animating anything. Use it to introduce a pause before the next step on the same track, without affecting steps on parallel tracks.

Color in the timeline: **gray**.

---

## Step dialog field reference

Double-click any step block to open the Edit Step dialog.

### Identity

| Field | Description |
|-------|-------------|
| **Label** | Name shown inside the step block in the timeline. |

### Timing

| Field | Description |
|-------|-------------|
| **Duration (s)** | Length of one loop playthrough in seconds. |
| **Time Scale** | Speed multiplier. `2.0` plays twice as fast; `0.5` plays at half speed. |
| **Loops (-1=∞)** | Number of times this step repeats. `-1` = infinite (only meaningful when the parent animation also loops). |
| **Ease Type** | Easing curve applied to the interpolation. Choices include Linear, Ease In/Out Sine/Quad/Cubic/Quart/Quint/Expo/Circ/Back/Elastic/Bounce and combinations. A small preview curve is drawn on the step block when it is wide enough. |
| **Loop Type** | `Restart` — repeats from the start each loop. `Ping Pong` — alternates forward and backward. |

### Target

| Field | Description |
|-------|-------------|
| **Parameter Name** | Material parameter name. Required for Float (Material Scalar) and Color (Material Vector) steps. |

### Appearance

| Field | Description |
|-------|-------------|
| **Step Color (override)** | Click the swatch to pick a custom color for this step block. When alpha > 0 the custom color replaces the default type color. Click **Reset** to revert to the type color (sets alpha to 0). |

### Type-specific fields

**Vector / Rotator steps:**

| Field | Description |
|-------|-------------|
| **From** | Start value. Hidden when **From Current** is checked. |
| **To** | End value. |
| **From Current** | When checked, the component's actual value at the moment the step starts is used as the From value, so the animation always picks up from where the component is. |

**Float steps:**

| Field | Description |
|-------|-------------|
| **From** | Starting scalar value (ignored when **From Current** is checked). |
| **To** | Ending scalar value. |
| **From Current** | Reads the live material parameter value when the step begins. |

**Color steps:**

| Swatch | Description |
|--------|-------------|
| **From (click)** | Click to open a color picker for the starting color. |
| **To (click)** | Click to open a color picker for the ending color. |

---

## Playback API reference

All nodes are available in Blueprint on a `UQuickTweenDirectorPlayer` reference.

### Control nodes

| Node | Description |
|------|-------------|
| `Play` | Start or resume playback. Calls `Build` automatically on the first play. |
| `Pause` | Pause at the current position. Call `Play` again to resume. |
| `Reverse` | Toggle playback direction (forward ↔ backward). |
| `Restart` | Reset to Idle state. Call `Play` to begin from the start. |
| `Complete` | Jump immediately to the end state and fire `OnComplete`. |
| `Kill` | Stop and unregister the player. Do not use the reference after this. |

### Query nodes

| Node | Returns |
|------|---------|
| `Get Is Playing` | `true` while ticking forward or backward. |
| `Get Is Completed` | `true` after the final loop finishes. |
| `Get Is Pending Kill` | `true` after `Kill` is called. |
| `Get Is Reversed` | `true` when playing backward. |
| `Get Elapsed Time` | Total seconds elapsed since playback began (across all loops). |
| `Get Loop Duration` | Duration of one loop in seconds (= asset total duration). |
| `Get Total Duration` | `Loop Duration × Loops`. |
| `Get Current Loop` | Zero-based index of the current loop. |
| `Current Local Time` | Position within the current loop in seconds (updated every tick). |

### Tween access

| Node | Description |
|------|-------------|
| `Get Tween By Step Id (FGuid)` | Returns the `UQuickTweenBase` for a specific step. Must be called after `Build` / `Play`. Cast to the concrete tween type to access type-specific callbacks. |
| `Get Tween By Label (FString)` | Returns the first tween whose label matches. Useful when you gave the step a known label in the dialog. |

### Slot binding

| Node | Description |
|------|-------------|
| `Bind Slot (Name, Object)` | Manually bind a slot name to an Actor or SceneComponent. Must be called before `Play` / `Build`. Overrides auto-binding for that slot. |
| `Build (WorldContext)` | Force a rebuild of the internal tween list. Called automatically by `Play` if not yet built. Call manually if you need to inspect tweens before playing. |

---

## Combining with other sequences

`UQuickTweenDirectorPlayer` implements the full `UQuickTweenable` interface, so it can be nested inside any QuickTween sequence:

```
[Create Sequence]
    │
    ├──► [Append] ← Director Player A   ← plays A, then B sequentially
    └──► [Append] ← Director Player B
```

Or play a Director animation in parallel with other tweens:

```
[Create Sequence]
    │
    ├──► [Join] ← Director Player       ← plays together
    └──► [Join] ← Some Float Tween
```

The Director Player behaves identically to a `UQuickTweenSequence` from the parent sequence's perspective.
