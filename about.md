# Jump Guide

A clean side timing bar for Geometry Dash that shows you **exactly when to press and when to hold**.

Copy a section of your level in the editor, play it, and follow the bar: segments slide toward a trigger line, turn **green** when you press on time, and keep growing for as long as you hold.

## What you get

- **A timing bar on the side of the screen** while you play, on the left or the right.
- **Tap segments** for obstacles. When a segment touches the line, press to jump. Hit it and it turns green, miss it and it turns red.
- **Hold segments** marked with arrows. Keep the button down while the segment crosses the line. The part you held fills in green, and the part you let go of turns red.
- **Speed-aware movement.** The bar follows the speed of the level, so it moves faster when the level gets faster and slower when it slows down.
- **A streak counter** at the top of the bar that counts your clean presses in a row.
- **A polished look** with neon glow, a scrolling grid that moves with your speed, a ripple on every successful press, and a soft fade at the top so segments appear out of the dark.

## How to set it up

1. **Mark your hold sections.** In the editor, place blocks over every part where the player must hold and set them to color channel **1000**. Any color works, dark grey looks best.
2. **Import your layout.** Select the spikes and the hold blocks (Select All is fine) and tap **Copy**. You will see a message like *"Jump Guide: 24 segments saved"*.
3. **Play the level.** Open the same level (the level name must match) and the bar appears on the side of the screen.

Copy again whenever you change the level, or when you change the spike IDs, the hold color channel or the merge gap in the settings.

## How segments are built

- Every spike from the **Spike Object IDs** list becomes part of a tap segment.
- Spikes that are close together are merged into one longer segment. The **Merge Gap** setting controls how close is "close".
- Blocks on the **hold color channel** are merged into one hold segment per continuous run.
- Ceiling and sideways spikes are ignored by default. Turn off **Upright Spikes Only** to include them.

## Settings

- **Enabled**: turn the bar on or off.
- **Bar Side**: left or right.
- **Edge Margin**, **Bar Height**, **Bar Width** and **Opacity**: size and position of the bar.
- **Accent Color**: Cyan, Magenta, Lime, Amber or Violet.
- **View Distance**: how far ahead the bar looks. A lower value makes segments move faster.
- **Press Offset**: how early a segment reaches the line before the obstacle, in milliseconds.
- **Glow Effects** and **Scrolling Grid**: visual extras you can switch off.
- **Spike Object IDs**, **Upright Spikes Only**, **Hold Marker Color Channel** and **Merge Gap**: control what counts as an obstacle or a hold.

## Good to know

- Jump Guide is a **practice and learning aid**. It does not press anything for you and does not change how your level plays.
- It is built for **Geometry Dash 2.2081**. Newer game versions may not work until Geode officially supports them.
- Your imported data is saved per level name, so you can set up several levels and switch between them.

Found a problem or want a new feature? Let the developer know.
