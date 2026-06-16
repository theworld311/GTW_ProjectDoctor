# Project Doctor — Fab Marketplace Listing

---

## Short Description (150 chars max)
Asset quality scanner for Unreal Engine 5. Detects naming issues, missing LODs, broken assets and more — inside a dockable editor panel.

---

## Long Description

**Stop shipping broken assets. Catch problems before they catch you.**

Project Doctor scans your entire Unreal Engine project in seconds and gives you a clear, actionable report of every asset quality issue — naming violations, missing collisions, texture problems, Blueprint compile errors, broken material instances, and much more.

Everything runs inside the Unreal Editor. No external tools, no command line, no setup.

---

### How it works

1. Open the **Project Doctor** panel from the Tools menu
2. Click **Scan /Game** (or choose a specific folder)
3. Review findings filtered by Errors, Warnings, and Info
4. **Double-click** any finding to jump directly to the asset in the Content Browser
5. Open the **HTML report** in your browser for a full formatted view

---

### What gets checked

**Textures**
- Very large textures (4K/8K warnings)
- Non-power-of-two dimensions
- sRGB enabled on data/mask textures
- Normal map naming mismatch
- Mip generation disabled

**Static Meshes**
- Missing simple collision
- Single LOD without Nanite
- Lightmap UV on channel 0

**Skeletal Meshes**
- Missing LODs
- Missing Physics Asset
- Missing Skeleton assignment

**Materials & Instances**
- Two-sided flag (doubles render cost)
- Translucency warnings
- Materials with no instances
- Material Instances with no parent

**Blueprints, AnimBPs, WidgetBPs**
- Compile errors
- Deprecated parent class
- Variables with no category

**Sounds**
- Long SoundWaves without streaming
- Naming convention

**Niagara & Particle Systems**
- Naming convention
- Missing fixed bounds

**All asset types**
- 18 naming prefix rules (SM_, SK_, T_, M_, MI_, BP_, ABP_, WBP_, A_, AS_, NS_ and more)
- Stale redirectors that block packaging

---

### Clean dockable UI

- Docks anywhere in your editor layout
- Live summary cards: Assets Scanned, Errors, Warnings, Info
- One-click filters: All / Errors / Warnings / Info
- Double-click any finding → Content Browser navigates to that asset
- Open HTML report in browser with one click
- Export to CSV and JSON for pipeline integration

---

### Technical Details

- **Engine:** Unreal Engine 5.4, 5.5, 5.7, 5.8
- **Type:** Editor Plugin (does not ship with your game)
- **Language:** C++ (Slate UI) + Python (scanner)
- **Dependencies:** Python Script Plugin (included with UE5, enabled by default)
- **Network:** No internet connection required

---

### Perfect for

- Solo developers preparing a project for release
- Studios enforcing asset standards across a team
- Technical artists doing asset reviews
- Anyone publishing on Fab or the Epic Marketplace

---

## Tags
asset quality, asset validator, naming convention, LOD checker, texture audit, blueprint validator, project cleanup, editor tool, pipeline, QA

---

## Category
Code Plugins → Editor Extensions
