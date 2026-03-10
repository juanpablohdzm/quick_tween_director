\mainpage Welcome to QuickTweenDirector

# Welcome to QuickTweenDirector

**QuickTweenDirector** is a visual timeline editor and runtime playback system built on top of the **QuickTween** plugin. It lets you design multi-track tween animations directly inside the Blueprint editor, store them as data assets, and play them back at runtime with just a couple of Blueprint nodes.

Think of it as a lightweight in-editor Sequencer that lives inside your Blueprint, produces a reusable animation asset, and drives everything through the QuickTween engine underneath.

> **Requires:** The **QuickTween** plugin must be installed and enabled in your project alongside this one.

---

## Why QuickTweenDirector?

If you have ever had to coordinate several tweens across multiple components — moving a mesh, fading a light, and scrolling a material UV all at the same time — you know how quickly that gets messy. You end up with a pile of BeginPlay nodes, hard-coded delays, and timing values scattered all over the place.

QuickTweenDirector fixes that by giving you a proper timeline to work in. You place colored step blocks on component tracks, drag them around to adjust timing, and when you are done you have a clean asset that you can drop into any Blueprint that has the same component setup.

Some of the things it makes straightforward:

- Designing animations visually instead of guessing delay values
- Reusing the same animation asset across multiple Blueprints
- Reacting to playback events like `OnComplete` or per-step `OnStepBegin` / `OnStepEnd`
- Exporting animations to JSON for version control or sharing between projects
- Combining a Director animation with other QuickTween sequences

---

## Installation

> If you installed QuickTweenDirector from the Fab Store, it should already appear in your Vault. If not, restart the launcher to refresh your library.

1. Copy the `QuickTweenDirector` folder into your project's `Plugins/` directory, next to the `QuickTween` folder:

```
YourProject/
  Plugins/
    QuickTween/
    QuickTweenDirector/
```

2. Right-click your `.uproject` file and select **Generate Visual Studio Project Files**.
3. Open your project. Unreal will ask you to compile the new plugin — click **Yes**.
4. Go to **Edit → Plugins**, search for **QuickTweenDirector**, and make sure it is enabled.
5. Restart the editor if prompted.

That is all you need. No C++ changes are required to start using the plugin in Blueprints.

---

## Opening the Director Panel

The Director editor lives inside the **Blueprint editor** — it is not a separate Content Browser tool.

1. Open any **Actor Blueprint**.
2. In the Blueprint editor toolbar, click the **Director** button.
3. The **QuickTween Director** panel opens as a dockable tab. You can drag it anywhere — below the viewport, next to the event graph, or as a floating window.

The panel automatically tracks whichever Actor Blueprint you have focused last, so switching between Blueprints will switch the panel's context too.

---

## Creating Your First Animation

Once the Director panel is open and you have an Actor Blueprint focused:

1. Click **+ New Animation** in the left column.
2. A `UQuickTweenDirectorAsset` is created alongside your Blueprint in the Content folder, and a Blueprint variable pointing to it is added automatically.
3. Double-click the entry to rename it to something meaningful.

From there, click the animation in the list to open its timeline and start adding tracks and steps.

---

## The Timeline at a Glance

The timeline is made up of **tracks** (one per component) and **steps** (the colored blocks that represent individual tween animations).

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

- **Right-click** an empty area on a track to add a step.
- **Left-click and drag** a step to move it in time.
- **Drag the right edge** of a step to resize its duration.
- **Double-click** a step to edit its values.
- Enable **Snap** in the toolbar to align steps to 0.05 s grid boundaries.

Tracks run in parallel at runtime, so you can overlap steps freely across different components.

---

## Playing the Animation at Runtime

Because the Director panel already added a Blueprint variable for your animation, using it at runtime is as simple as two nodes:

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

Blueprint components are **automatically bound by name** — you do not need to call `Bind Slot` for components that belong to the same Blueprint. For material steps, you will need to create a `UMaterialInstanceDynamic` and bind it manually before calling `Play`.

---

## Requests and Bug Reports

If there is something you wish the plugin could do, or if you run into a problem, please get in touch:

- **Feature requests:** send an email to `mosti.dev@gmail.com` with the subject **Request - \<Feature Name\>** and a description of what you need.
- **Bug reports:** send an email to `mosti.dev@gmail.com` with the subject **Bug - \<Descriptive Name\>**, including your Unreal Engine version, a description of the issue, a callstack if available, and steps to reproduce.

---

Thanks for using **QuickTweenDirector**. If you have any questions, feel free to reach out at `mosti.dev@gmail.com`.
