# AI Asset Namer — Fab Marketplace Listing

---

## Short Description (150 chars max)
Rename assets automatically with AI. Correct UE5 prefixes (SM_, T_, BP_...) suggested for any selection or folder — review and apply in one click.

---

## Long Description

**Messy asset names? Fix your whole project in minutes.**

AI Asset Namer scans your selected assets or folders and uses AI (Claude or OpenAI — your choice, your API key) to suggest correct names following the standard Unreal Engine naming conventions. Review every suggestion side by side, uncheck what you don't want, and apply the rest with one click.

No more `finalMesh_2_new` in a shipping project.

---

### How it works

1. Open **AI Asset Namer** from the Tools menu
2. Pick your provider (Claude or OpenAI) and paste your API key
3. **Load Selected Assets** from the Content Browser, or **Load Folder...**
4. Click **Suggest Names (AI)** — suggestions appear next to the current names
5. Review, uncheck exceptions, **Apply Selected**

Renames go through the engine's Asset Tools, so redirectors and references are handled properly.

---

### What gets fixed

- Missing prefixes: `rock` → `SM_Rock`, `hero_diffuse` → `T_HeroDiffuse`
- Wrong prefixes: `Mesh_Rock` → `SM_Rock`
- Casing: `sm_old_barrel` → `SM_OldBarrel`
- 20+ asset types: StaticMesh, SkeletalMesh, Texture2D, Material, Material Instance, Blueprint, AnimBP, WidgetBP, SoundWave, SoundCue, AnimSequence, DataTable, DataAsset, Enum, Struct, Niagara and more

---

### Privacy & Cost

- Only asset **names and types** are sent to the AI — never contents or files
- Bring your own API key (Anthropic or OpenAI); keys are stored locally in your user settings, never in the project
- Batches of 100 assets cost a fraction of a cent with the default models

---

### Technical Details

- **Engine:** Unreal Engine 5.4, 5.5, 5.7, 5.8
- **Type:** Editor Plugin (does not ship with your game)
- **Language:** C++ (Slate UI)
- **Dependencies:** Python Script Plugin (included with UE5, enabled by default)
- **Network:** Internet connection + API key required for suggestions only

---

### Perfect for

- Cleaning up prototypes before they become products
- Studios enforcing naming standards across a team
- Marketplace/Fab sellers preparing asset packs
- Anyone inheriting a messy project

---

## Tags
asset naming, naming convention, rename tool, AI tool, project cleanup, asset organization, editor tool, pipeline, batch rename, QA

---

## Category
Code Plugins → Editor Extensions
