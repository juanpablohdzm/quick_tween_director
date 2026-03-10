\mainpage Welcome to QuickTweenDirector

# Welcome to QuickTweenDirector

**QuickTweenDirector** is a visual timeline editor and runtime playback system built on top of the **QuickTween** plugin. It lets you design multi-track tween animations directly inside the Blueprint editor, store them as reusable data assets, and play them back at runtime with just a couple of Blueprint nodes — no code required.

Think of it as a lightweight in-editor Sequencer that lives inside your Blueprint. You place colored step blocks on component tracks, arrange their timing visually, save the result as an asset, and then drive it all from a single `Create Director Player → Play` call at runtime.

> **Requires:** The **QuickTween** plugin must be installed and enabled in your project alongside this one.

---

## Why QuickTweenDirector?

If you have ever built a complex animation using QuickTween alone, you know the problem. You end up with a wall of BeginPlay nodes, magic delay numbers, and timing logic spread across multiple event graphs. When something needs to change, you have to hunt through all of it.

QuickTweenDirector solves this by giving you a proper timeline. You can see all your component animations at once, drag steps around to get the timing right, and when you are done you have a clean asset that can be reused across multiple Blueprints.

Here is what it makes easy:

- Designing animations visually instead of guessing delay values
- Running multiple component animations in parallel without manually coordinating them
- Storing animations as data assets that serialize with your project
- Reusing the same animation asset from any Blueprint that has matching components
- Reacting to playback events — globally or per individual step
- Exporting and importing animations as JSON for version control or sharing
- Nesting Director animations inside larger QuickTween sequences

---

## Installation

1. Copy the `QuickTweenDirector` folder into your project's `Plugins/` directory, next to the `QuickTween` folder:

```
YourProject/
  Plugins/
    QuickTween/
    QuickTweenDirector/
```

2. Right-click your `.uproject` file and select **Generate Visual Studio Project Files**.
3. Open your project. Unreal will ask you to compile the new plugin — click **Yes**.
4. Go to **Edit → Plugins**, search for **QuickTweenDirector**, and confirm it is enabled.
5. Restart the editor if prompted.

No additional C++ setup is needed to use the plugin from Blueprints.

---

## Opening the Director Panel

The Director editor is embedded inside the **Blueprint editor**. It is not a standalone window or Content Browser tool.

1. Open any **Actor Blueprint** by double-clicking it in the Content Browser.
2. In the Blueprint editor toolbar, click the **Director** button.
3. The **QuickTween Director** panel opens as a dockable tab. Drag it wherever is comfortable — below the viewport, next to the event graph, or as a floating window.

The panel tracks the last-focused Actor Blueprint automatically. If you have multiple Blueprint editors open, clicking **Director** from the right one switches the panel's context.

---

## Creating an Animation Asset

With the Director panel open and an Actor Blueprint focused:

1. Click **+ New Animation** in the left column.
2. A `UQuickTweenDirectorAsset` file is created alongside your Blueprint in the Content folder, and a Blueprint variable referencing it is added automatically.
3. Double-click the animation entry to rename it.
4. Click the **✕** button next to an entry to remove the variable reference. The asset file stays on disk — only the variable link is removed.

You can have as many animations per Blueprint as you need. All of them appear in the left column and each one has its own independent timeline.

---

## The Timeline Editor

Clicking an animation in the left column opens its timeline. This is where you build the animation.

```
┌─ Toolbar ─────────────────────────────────────────────────────────────────────┐
│ [Save] [+ Add Track] | Duration: 4.50s | [Snap] [Fit] ──○──  [Export] [Import]│
├────────────────────────┬──────────────────────────────────────────────────────┤
│ TIMELINE               │  0s      1s      2s      3s      4s                  │
├─ StaticMesh            │  ████████ Relative Location ████│                    │
│   StaticMeshComponent  │                                                       │
├─ Light                 │          ████ Material Float ████████                 │
│   PointLightComponent  │                                                       │
└────────────────────────┴──────────────────────────────────────────────────────┘
```

The total animation **Duration** shown in the toolbar is calculated automatically as the furthest step end time across all tracks. You do not set it manually.

---

### Toolbar Reference

| Control | What it does |
|---------|--------------|
| **Save** | Saves the Director asset to disk. |
| **+ Add Track** | Opens the component picker and adds a new track. |
| **Duration badge** | Shows the total animation duration. Read-only — it updates as you add or move steps. |
| **Snap** | Toggle. When active, drag and resize operations snap to 0.05 s boundaries. Turns orange when enabled. |
| **Fit** | Adjusts zoom so the entire animation fits in the visible area. |
| **Zoom slider** | Drag right for more detail, left to see more of the timeline. Shown as a percentage of the 80 px/s default. |
| **Export JSON** | Saves all tracks and steps to a `.json` file. |
| **Import JSON** | Replaces current tracks and steps from a previously exported `.json` file. |

---

### Adding Tracks

Each **track** targets one component from your Blueprint's component hierarchy. Tracks run in parallel at runtime — steps on different tracks can overlap freely.

**To add a track:**
1. Click **+ Add Track** in the toolbar.
2. The **Select Component** dialog shows the component tree of your Blueprint.
3. Select a component and click **OK**.

> Only Blueprint SCS components (the ones you added yourself) appear in the picker. Inherited C++ native components can still be animated by calling `Bind Slot` manually at runtime.

**To reorder tracks:**
Use the **▲ / ▼** arrow buttons on the left edge of each track label. The change is undoable with Ctrl+Z.

**To delete a track (and all its steps):**
Click the **✕** button on the track label. This is undoable.

---

### Adding Steps

A **step** is a colored block on a track that represents one tween animation. Steps on the same track play sequentially relative to each other based on their start times, but steps across different tracks always play in parallel.

**To add a step:**
1. **Right-click** on an empty area of a track row.
2. The **Add Step** submenu appears, filtered to step types that make sense for that component class.
3. Select a type. The **Edit Step** dialog opens so you can configure values and timing.
4. Click **Apply**. The step appears on the track as a colored block.

The start time of the new step is placed at the position where you right-clicked, snapping to 0.05 s if Snap is enabled.

---

### Moving Steps

- **Left-click and drag** a step block horizontally to change its start time.
- With **Snap** enabled, the start time snaps to 0.05 s boundaries.
- Release to commit. Undoable.

---

### Resizing Steps

- Hover over the **right edge** of a step block. The cursor changes to a resize arrow.
- **Left-click and drag** to change the step's duration.
- With **Snap** enabled, the duration snaps to 0.05 s boundaries.
- Release to commit. Undoable.

---

### Editing Step Values

**Double-click** any step block to re-open the Edit Step dialog and change any of its settings — timing, easing, target values, or appearance.

---

### Multi-Selecting Steps

- **Ctrl + left-click** steps to add or remove them from the selection. Selected steps show a white outline.
- Click on empty track space (without Ctrl) to clear the selection.
- Right-clicking with multiple steps selected adds a **Delete Selected** option to the context menu.

---

### Copying and Pasting Steps

- **Right-click** a step → **Copy Step** — copies it to the clipboard.
- **Right-click** on empty space in any track → **Paste Step** — pastes a copy at the click position on that track. The pasted step gets a new ID but inherits everything else: type, duration, easing, and all values.

This is useful for duplicating a step across tracks or creating similar steps with slightly different timing.

---

### Deleting Steps

| What | How |
|------|-----|
| Delete one step | Right-click it → **Delete Step** |
| Delete multiple steps | Select them with Ctrl+click, then right-click → **Delete Selected** |
| Delete a whole track | Click the **✕** on the track label (also removes all steps on it) |

All deletions are undoable.

---

### Grid Snapping

Click the **Snap** toggle in the toolbar to enable grid snapping. The snap increment is fixed at **0.05 seconds**. When active:

- Dragging a step snaps its start time to the nearest 0.05 s boundary.
- Resizing a step snaps its duration to 0.05 s boundaries.
- New steps placed via right-click snap their start time.

Snap is shown in orange in the toolbar when it is on.

---

### Ruler Scrubbing

The green playhead on the ruler shows your current position in the timeline.

- **Click or drag on the ruler** to move the playhead to any time.
- The playhead is a visual reference only — it does not drive live actor movement in the editor.

To actually preview the result, use **Play in Editor (PIE)** with `Create Director Player → Play` hooked up in BeginPlay. Easing curve shapes are also previewed visually inside step blocks when the block is wide enough — a subtle curve line shows the shape of the chosen easing function.

---

### Zoom and Fit

- **Zoom slider**: drag right to zoom in (see more detail per second), left to zoom out (see more of the animation).
- **Fit**: one click to set the zoom level so the full animation duration fits the visible area. Useful after adding long steps.

---

### Exporting to JSON

Click **Export JSON** in the toolbar to write the entire asset to a `.json` file. The export is **lossless** — it contains every field needed to fully restore the asset:

- Asset name, loop count, total duration
- All tracks (GUID, label, component variable name)
- All steps (GUID, track GUID, label, type, start time, duration, time scale, loops, loop type, easing, ease curve path, slot name, parameter name, all type-specific values, custom color)

**When to use this:**
- Diffing animation changes in source control
- Sharing animations between projects
- Backing up before a large restructure

> The JSON file is not used at runtime. It is an editor-only exchange format.

---

### Importing from JSON

Click **Import JSON** and pick a previously exported `.json` file. If the current asset already has content, you will be asked to confirm before anything is replaced.

The importer validates every entry and skips anything that is malformed, showing a warning for each issue. After import, a summary dialog lists how many tracks and steps were imported and all warnings.

**Common import warnings:**

| Warning | Cause | Fix |
|---------|-------|-----|
| `tracks[N]: invalid GUID` | The id field in the JSON is corrupt | Restore from an unmodified export |
| `steps[N]: trackId does not match any imported track` | The step references a track that failed to import | Fix the track entry in the JSON |
| `Track 'X': component 'Y' not found in the current Blueprint` | Animation was made for a different Blueprint or the component was renamed | Rename the component or call `Bind Slot` manually at runtime |
| `Failed to parse JSON` | File is corrupt or was not created by Export JSON | Re-export from the original asset |

---

### Saving

Click **Save** in the toolbar (or Ctrl+S while the Director panel is focused) to save the Director asset. The asset is a standard `UDataAsset` file in your Content folder. The Blueprint variable pointing to it is saved when you save the Blueprint normally.

---

## Step Types

When you right-click a track to add a step, the available types depend on the component class of that track. Here is the full list.

---

### Location and Scale (Vector)

These animate an `FVector` property on any `USceneComponent`. The timeline block color is **orange** for location and **amber** for scale.

**Absolute ("To") — animates from a start value to a fixed target:**

| Step Type | What it drives |
|-----------|---------------|
| Relative Location | `SetRelativeLocation` |
| World Location | `SetWorldLocation` |
| Relative Scale | `SetRelativeScale3D` |
| World Scale | `SetWorldScale3D` |

**Relative ("By") — adds a delta to the component's value at build time:**

| Step Type | What it drives |
|-----------|---------------|
| Relative Location By | `SetRelativeLocation` (current + delta) |
| World Location By | `SetWorldLocation` (current + delta) |
| Relative Scale By | `SetRelativeScale3D` (current + delta) |
| World Scale By | `SetWorldScale3D` (current + delta) |

> **"To" vs "By":** Use **To** when you want to animate to a specific position in the world. Use **By** when you want the animation to work relative to wherever the component happens to be — for example "move 200 units forward from wherever it starts."

---

### Rotation (Rotator)

Animates an `FRotator` on a `USceneComponent`, always taking the shortest angular path. Timeline color: **teal**.

**Absolute ("To"):**

| Step Type | What it drives |
|-----------|---------------|
| Relative Rotation | `SetRelativeRotation` |
| World Rotation | `SetWorldRotation` |

**Relative ("By"):**

| Step Type | What it drives |
|-----------|---------------|
| Relative Rotation By | `SetRelativeRotation` (current + delta) |
| World Rotation By | `SetWorldRotation` (current + delta) |

---

### Material Scalar Parameter (Float)

Animates a scalar (float) parameter on a `UMaterialInstanceDynamic`. Timeline color: **blue**.

- **Parameter Name** must exactly match the parameter name in your material.
- The slot must be manually bound to a `UMaterialInstanceDynamic` before `Play`. See the Slot Bindings section.

---

### Material Vector Parameter (Linear Color)

Animates an RGB color vector parameter on a `UMaterialInstanceDynamic`. Timeline color: **purple**.

- **Parameter Name** must match the vector parameter in your material.
- Requires a manual `Bind Slot` to a `UMaterialInstanceDynamic`.

---

### Material Vector2D Parameter (UV)

Animates the **XY channels** of a material vector parameter on a `UMaterialInstanceDynamic` — ideal for UV offset and UV tiling animations. The ZW channels are preserved (snapshotted at build time). Timeline color: **green**.

- **Parameter Name** must match the vector parameter in your material.
- Requires a manual `Bind Slot` to a `UMaterialInstanceDynamic`.

---

### Material Scalar Integer

Animates a scalar parameter on a `UMaterialInstanceDynamic` using integer interpolation — the result is rounded to the nearest integer each tick. Use this for discrete states like texture atlas indices. Timeline color: **coral**.

- **Parameter Name** must match the scalar parameter in your material.
- Requires a manual `Bind Slot` to a `UMaterialInstanceDynamic`.

---

### Delay (Empty)

A no-op step that occupies time on a track without animating anything. Use it to introduce a pause before the next step on the same track, without affecting other tracks. Timeline color: **gray**.

---

## Edit Step Dialog — Field Reference

Double-click any step block to open the Edit Step dialog. Here is what every field does.

### Identity

| Field | Description |
|-------|-------------|
| **Label** | The name shown inside the step block on the timeline. Use something descriptive — you can also find a step by label using `Get Tween By Label` at runtime. |

### Timing

| Field | Description |
|-------|-------------|
| **Duration (s)** | How long one loop playthrough lasts, in seconds. |
| **Time Scale** | Speed multiplier. `2.0` plays twice as fast. `0.5` plays at half speed. The visual block width on the timeline reflects the real playback time after time scale is applied. |
| **Loops (min 1)** | How many times this step repeats. Minimum is `1`. Infinite loops (`-1`) are **not supported** inside a Director sequence because the player needs a known, finite total duration. |
| **Loop Type** | `Restart` — resets to the start value each loop. `Ping Pong` — alternates forward and backward. |
| **Ease Type** | The easing curve applied to the interpolation. Options include Linear, Sine, Quad, Cubic, Quart, Quint, Expo, Circ, Back, Elastic, and Bounce — each in In, Out, and InOut variants. A preview curve is drawn on the step block when it is wide enough. |
| **Ease Curve (override)** | Assign a `UCurveFloat` asset here to replace the Ease Type for this step. Clear it to fall back to the Ease Type dropdown. |

### Target

| Field | Description |
|-------|-------------|
| **Parameter Name** | The material parameter name. Required for Float, Color, Vector2D, and Integer steps. Must match exactly what is in your material. |

### Appearance

| Field | Description |
|-------|-------------|
| **Step Color (override)** | Click the swatch to pick a custom color for this step block. When alpha is greater than 0, the custom color replaces the default type color. Click **Reset** to go back to the type color (sets alpha to 0). |

### Type-Specific Fields

**Vector steps — absolute (To):**

| Field | Description |
|-------|-------------|
| **From (X, Y, Z)** | The starting value. Hidden when **From Current** is checked. |
| **To (X, Y, Z)** | The ending value. |
| **From Current** | When checked, the component's actual value at the moment the animation is built is used as the From value. Useful for animations that start from wherever the component currently is. |

**Vector steps — relative (By):**

| Field | Description |
|-------|-------------|
| **By / Delta (X, Y, Z)** | The amount added to the component's value at build time. The From value is always the snapshot taken when `Play` is first called — From and From Current are hidden. |

**Rotator steps — absolute (To):**

| Field | Description |
|-------|-------------|
| **From (Pitch, Yaw, Roll)** | Start rotation. Hidden when **From Current** is checked. |
| **To (Pitch, Yaw, Roll)** | End rotation. |
| **From Current** | Uses the component's current rotation at build time as the start. |

**Rotator steps — relative (By):**

| Field | Description |
|-------|-------------|
| **By / Delta (Pitch, Yaw, Roll)** | The rotation delta added to the component's current rotation at build time. |

**Float steps:**

| Field | Description |
|-------|-------------|
| **From** | Starting scalar value. Ignored when **From Current** is checked. |
| **To** | Ending scalar value. |
| **From Current** | Reads the live material parameter value when the animation is built. |

**Color steps:**

| Field | Description |
|-------|-------------|
| **From** | Click the color swatch to pick the starting color. |
| **To** | Click the color swatch to pick the ending color. |

**Vector2D steps:**

| Field | Description |
|-------|-------------|
| **From (X, Y)** | Starting XY value. Hidden when **From Current** is checked. |
| **To (X, Y)** | Ending XY value. |
| **From Current** | Reads the XY of the material vector parameter at build time. ZW channels are always preserved. |

**Integer steps:**

| Field | Description |
|-------|-------------|
| **From** | Starting integer value. Ignored when **From Current** is checked. |
| **To** | Ending integer value. |
| **From Current** | Reads the live material scalar at build time and rounds it to the nearest integer. |

---

## Playing the Animation at Runtime

### Basic Playback

Because the Director panel already added a Blueprint variable pointing at your animation asset, using it at runtime only takes two nodes.

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

Blueprint SCS components are **automatically bound by name** — no `Bind Slot` calls needed for components that belong to the same Blueprint.

---

### Storing the Player for Later Control

If you want to pause, reverse, or restart the animation after it has started, store the player in a variable:

```
[Event BeginPlay]
    │
    ▼
[Create Director Player] ──► [SET MyDirectorPlayer] ──► [Play]

[Event OnTriggerEnter]   ──► [GET MyDirectorPlayer] ──► [Pause]

[Event OnTriggerExit]    ──► [GET MyDirectorPlayer] ──► [Play]
```

This pattern works for anything — buttons, overlap events, timers, game state changes.

---

### Reacting When the Animation Finishes

Bind to `OnComplete` before calling `Play`:

```
[Create Director Player]
    │
    ├──► [Assign OnComplete ← My Callback Event]
    │
    └──► [Play]

[My Callback Event] ──► [Do something when animation ends]
```

You can also bind `OnStart`, `OnLoop`, `OnKilled`, and `OnUpdate` the same way.

---

### Reacting to Individual Step Boundaries

`OnStepBegin` fires when a step's time window becomes active. `OnStepEnd` fires when it becomes inactive. Both receive the **slot name** (the component variable name) and the **step ID** (a `FGuid`).

```
[Create Director Player]
    │
    ├──► [Assign OnStepBegin ← HandleStepBegin]
    ├──► [Assign OnStepEnd   ← HandleStepEnd]
    └──► [Play]

[HandleStepBegin]  ── inputs: Player, SlotName (FName), StepId (FGuid)
[HandleStepEnd]    ── same inputs
```

This is useful for triggering sound effects, particle systems, or other game events at precise moments in the animation without adding separate timers.

---

### Accessing a Specific Step's Tween

If you want to bind a callback to one specific step rather than all steps, get it by label:

```
[Create Director Player] ──► [Play]
                         ──► [Get Tween By Label "DoorOpen"] ──► [Assign OnComplete ← HandleDoorOpened]
```

Or by step ID (the `FGuid` shown in the Edit Step dialog):

```
[Create Director Player] ──► [Play]
                         ──► [Get Tween By Step Id] ──► [Assign OnComplete ← HandleSpecificStep]
```

Call these **after** `Create Director Player` and **before or after** `Play` — the tweens are created during `Build`, which is called automatically on the first `Play`.

---

## Slot Bindings

At runtime, the player calls `Build()` automatically the first time you call `Play`. Build walks through all the tracks and tries to find a matching component on the owning actor by name.

| Situation | Result |
|-----------|--------|
| Animation created for this Blueprint's component | Automatically bound — no action needed |
| `Bind Slot(Name, Object)` called before `Play` | Manual binding takes priority over the automatic one |
| Slot name is empty | Falls back to the world context object |
| Slot bound to a `UMaterialInstanceDynamic` | Required for all material step types |
| Component not found and no manual binding | Step is skipped; error is added to `Binding Errors` |

### Binding a Material Instance Dynamic

Material step types (Float, Color, Vector2D, Integer) cannot auto-bind because they need a `UMaterialInstanceDynamic`, not a component. You must create the MID yourself and bind it before calling `Play`:

```
[Event BeginPlay]
    │
    ▼
[Create Dynamic Material Instance]
    Source Material ← your material
    ──► MID

[Create Director Player] ──► Player

[Player → Bind Slot]
    Name   ← "MyMeshComponent"   (matches the track's slot name in the Director)
    Object ← MID

[Player → Play]
```

You can also apply the MID to your mesh in the same BeginPlay before or after binding — the order relative to `Bind Slot` does not matter as long as both happen before `Play`.

---

## Debugging Binding Errors

After `Build` runs (which happens automatically on first `Play`), you can read `Player → Binding Errors`. It is a `TArray<FString>` that lists every slot that failed to auto-bind:

```
Track 'Cube': slot 'Cube' could not be auto-bound — no matching component or property found on 'BP_MyActor'.
```

**Common causes and fixes:**

| Cause | Fix |
|-------|-----|
| Component was renamed in the Blueprint after the track was created | Rename it back, or recreate the track from the correctly named component |
| Animation was created for a different Blueprint | Call `Bind Slot` manually with the correct object before `Play` |
| Component is a C++ native component (not in the SCS tree) | Call `Bind Slot` manually |
| Material step slot not bound to a MID | Create a MID and call `Bind Slot` before `Play` |

A good practice during development is to add a branch after `Play` that checks if `Binding Errors` is non-empty and prints each error to the screen. This makes problems obvious in PIE without having to open the output log.

---

## Blueprint Events Reference

All events are `BlueprintAssignable` multicast delegates. Drag from the player reference and use **Assign Event** nodes.

| Event | Signature | When it fires |
|-------|-----------|---------------|
| `OnStart` | `(Player)` | The first `Play` from Idle state |
| `OnUpdate` | `(Player)` | Every tick while playing |
| `OnLoop` | `(Player)` | Each time the animation completes one full loop |
| `OnComplete` | `(Player)` | The final loop finishes |
| `OnKilled` | `(Player)` | `Kill` is called |
| `OnStepBegin` | `(Player, SlotName, StepId)` | A step's window becomes active |
| `OnStepEnd` | `(Player, SlotName, StepId)` | A step's window becomes inactive |

**Read-only playback properties** (available as `BlueprintReadOnly` on the player):

| Property | Type | Description |
|----------|------|-------------|
| `CurrentLocalTime` | `float` | Current position within the current loop, in seconds |
| `BindingErrors` | `TArray<FString>` | Populated after `Build` if any slots failed to bind |

---

## Playback API Reference

All of these are available in Blueprint on a `UQuickTweenDirectorPlayer` reference.

### Control Nodes

| Node | Description |
|------|-------------|
| `Play` | Start or resume playback. Calls `Build` automatically on the first play. |
| `Pause` | Freeze at the current position. Call `Play` again to resume from the same point. |
| `Reverse` | Toggle playback direction between forward and backward. |
| `Restart` | Reset to Idle state. Call `Play` to begin from the start again. |
| `Complete` | Jump immediately to the end state and fire `OnComplete`. |
| `Kill` | Stop and unregister the player. Do not use the reference after calling this. |

### Query Nodes

| Node | Returns |
|------|---------|
| `Get Is Playing` | `true` while ticking forward or backward |
| `Get Is Completed` | `true` after the final loop has finished |
| `Get Is Pending Kill` | `true` after `Kill` is called |
| `Get Is Reversed` | `true` when playing backward |
| `Get Elapsed Time` | Total seconds elapsed since playback began (across all loops) |
| `Get Loop Duration` | Duration of one loop in seconds — equals the asset's total duration |
| `Get Total Duration` | `Loop Duration × Loop Count` |
| `Get Current Loop` | Zero-based index of the current loop |
| `Current Local Time` | Position within the current loop in seconds, updated every tick |

### Tween Access Nodes

| Node | Description |
|------|-------------|
| `Get Tween By Step Id (FGuid)` | Returns the `UQuickTweenBase` for a specific step. Must be called after `Build` / `Play`. Cast to the concrete tween type to access type-specific callbacks. |
| `Get Tween By Label (FString)` | Returns the first tween whose label matches the string you provide. |

### Slot Binding Nodes

| Node | Description |
|------|-------------|
| `Bind Slot (Name, Object)` | Manually bind a slot name to an Actor, SceneComponent, or `UMaterialInstanceDynamic`. Must be called before `Play` or `Build`. Overrides auto-binding for that slot. |
| `Build (WorldContext)` | Force a rebuild of the internal tween list. Called automatically by `Play` if not yet built. Call manually if you need to inspect tweens before starting playback. |

---

## Common Use Case Examples

### Example 1 — A Door Opening Animation

You have a door Blueprint with a `DoorMesh` component. You want it to rotate 90 degrees over 0.8 seconds with an ease-out feel when the player interacts with it.

**In the Director:**
1. Open the door Blueprint and click **Director**.
2. Click **+ New Animation** and name it `Open`.
3. Click **+ Add Track**, select `DoorMesh`.
4. Right-click the track → **Relative Rotation** → set From to `(0, 0, 0)`, To to `(0, 90, 0)`, Duration `0.8s`, Ease Type `Ease Out Cubic`.
5. Click **Apply**, then **Save**.

**In BeginPlay or an interaction event:**

```
[On Interact]
    │
    ▼
[Create Director Player]  ← Asset: Open (your variable)
    World Context ← self
    │
    ▼
[Play]
```

---

### Example 2 — A Chest That Opens with Sound at a Specific Moment

You want to play a sound effect exactly when the lid reaches 45 degrees — halfway through a 1-second rotation.

**In the Director:**
1. Add a track for `LidMesh`, add a Relative Rotation step: From `(0,0,0)`, To `(0,0,90)`, Duration `1s`. Label it `LidRotate`.
2. Add a Delay step on the same track starting at 0.5s with duration 0.01s. Label it `SoundCue`.

**In Blueprint:**

```
[Create Director Player]
    │
    ├──► [Assign OnStepBegin ← HandleStepBegin]
    └──► [Play]

[HandleStepBegin]
    SlotName ──► [Switch on Name]
        "LidMesh" ──► [Get Tween By Label "SoundCue"] ──► [If valid] ──► [Play Sound]
```

Or more directly, use `OnStepBegin` and filter by the step label using `Get Tween By Label` to confirm it is the right step, then play the sound.

---

### Example 3 — A UI Panel That Slides In and Fades Simultaneously

You have a widget component with a background mesh and a spotlight. You want the mesh to slide in from the left over 0.5s while the light fades up over 0.4s starting 0.1s into the animation.

**In the Director:**
1. Add a track for `BackgroundMesh`. Right-click → **Relative Location** → From `(-500, 0, 0)`, To `(0, 0, 0)`, Duration `0.5s`, Ease `Ease Out Quint`. Place it at time 0.
2. Add a track for `SpotLight`. Right-click → **Material Scalar Parameter** → Parameter Name `Brightness`, From `0.0`, To `1.0`, Duration `0.4s`. Place it at time 0.1s.

Both tracks run in parallel automatically. No extra coordination needed.

---

### Example 4 — Looping an Animation and Stopping It Later

```
[Event BeginPlay]
    │
    ▼
[Create Director Player]
    │
    ├──► [SET IdleAnimation]
    └──► [Play]

[Event OnPlayerNearby]
    │
    ▼
[GET IdleAnimation] ──► [Complete]   ← snaps to end state
                    ──► [Kill]       ← or Kill to stop immediately without snapping
```

Because Director Player steps must have a finite loop count, the animation loops naturally when you set the asset's loop count to more than 1 (or handle looping by re-calling `Play` in `OnComplete`).

---

### Example 5 — Nesting a Director Animation Inside a Sequence

You can use a Director Player as one step inside a larger `UQuickTweenSequence`, mixing it with individual tweens:

```
[Create Director Player]  ← your animation asset
    │
    ──► Player

[Create Sequence]
    │
    ├──► [Append] ← Player         ← runs Director animation first
    ├──► [Append] ← Float Tween    ← then runs this after it finishes
    └──► [Play]
```

Or in parallel:

```
[Create Sequence]
    │
    ├──► [Join] ← Player           ← runs together
    ├──► [Join] ← Camera Tween     ← at the same time
    └──► [Play]
```

The Director Player behaves exactly like a `UQuickTweenSequence` from the parent's perspective.

> **Important:** When nesting, every step inside the Director animation must have a finite loop count (minimum 1). The parent sequence needs a known total duration to calculate seek times.

---

## Tips for Juniors

A few things that are easy to miss when getting started:

- **Nothing plays in editor.** The timeline is for authoring only. To see the result, you must run PIE with `Create Director Player → Play` in BeginPlay.
- **The Duration badge is read-only.** It reflects the furthest step end time. To make the animation longer, add a step or extend an existing one further out.
- **Snap makes life easier.** Turn it on early and keep it on. It keeps steps aligned to clean boundaries and makes timing predictable.
- **Export before big changes.** Use Export JSON as a backup before reorganizing tracks or doing large edits. You can always import it back.
- **Check Binding Errors in PIE.** If something is not animating, the first thing to check is the `Binding Errors` array on the player. It will tell you exactly which slots failed and why.
- **Infinite loops are not allowed inside a Director.** If a step needs to loop forever, you cannot do it inside the timeline. Instead, use a loop count of 1 in the step and handle repeating at the Director level, or use a regular QuickTween alongside the Director.
- **Material steps always need a MID.** You cannot animate a material parameter directly on a Static Mesh component. You must create a `UMaterialInstanceDynamic`, apply it to the mesh, and bind it via `Bind Slot` before `Play`.

---

## Requests and Bug Reports

If there is something you wish the plugin could do, or if you run into a problem:

- **Feature requests:** email `mosti.dev@gmail.com` with subject **Request - \<Feature Name\>** and a description of what you need.
- **Bug reports:** email `mosti.dev@gmail.com` with subject **Bug - \<Descriptive Name\>**, including your Unreal Engine version, a description of the issue, a callstack if available, and steps to reproduce.

---

Thanks for using **QuickTweenDirector**. If you have any questions, feel free to reach out at `mosti.dev@gmail.com`.
