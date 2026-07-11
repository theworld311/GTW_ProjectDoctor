# AI Asset Namer

Rename Unreal Engine assets automatically using AI. Select assets or a folder, get naming suggestions that follow the standard UE5 prefix conventions (SM_, T_, M_, BP_...), review them side by side, and apply with one click.

Powered by Claude (Anthropic) or OpenAI — you choose the provider and use your own API key.

---

## Requirements

- Unreal Engine 5.4+ (built and tested on 5.7)
- **Python Script Plugin** enabled (ships with UE5, enabled by default)
- An API key from [Anthropic](https://platform.claude.com/) or [OpenAI](https://platform.openai.com/)

---

## Quick Start

1. Enable the plugin: **Edit → Plugins → AI Asset Namer** → restart the editor
2. Open the panel: **Tools → AI Asset Namer**
3. Choose your **Provider** (Claude or OpenAI) and paste your **API key** — it is stored locally in your per-project user settings, never embedded in the plugin or your project
4. Select assets in the Content Browser and click **Load Selected Assets** (or use **Load Folder...** with a path like `/Game/Characters`)
5. Click **Suggest Names (AI)** — each asset gets a suggested name following UE5 conventions
6. Review the list. Uncheck anything you don't want renamed
7. Click **Apply Selected** — assets are renamed in place with redirectors handled by the engine

---

## What it fixes

- Missing prefixes (`rock` → `SM_Rock`, `hero_diffuse` → `T_HeroDiffuse`)
- Wrong prefixes (`Mesh_Rock` → `SM_Rock`)
- Inconsistent casing (`sm_old_barrel` → `SM_OldBarrel`)
- 20+ asset types covered: StaticMesh, SkeletalMesh, Texture2D, Material, Material Instance, Blueprint, AnimBP, WidgetBP, Sound, DataTable, Niagara and more

Suggested names are sanitized to valid asset-name characters before being applied.

---

## Privacy & Cost

- Only asset **names and types** are sent to the AI provider — never asset contents, textures, or project files
- You pay the provider directly through your own API key; a batch of 100 assets costs a fraction of a cent with the default models
- Default models: `claude-haiku-4-5-20251001` (Claude) / `gpt-4o-mini` (OpenAI) — configurable in **Project Settings → Plugins → AI Asset Namer**

---

## Support

- Issues and questions: https://github.com/theworld311/GTW_ProjectDoctor/issues

---

Copyright (c) 2026 GTW Dev. All Rights Reserved.
