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

---

## Frequently Asked Questions

**Q: Will this rename ALL my assets?**  
A: No. You load specific assets or folders, review every suggestion, and uncheck anything you don't want renamed. Nothing happens without your approval.

**Q: What if the AI suggests a bad name?**  
A: You see all suggestions before applying. Uncheck bad ones or edit them directly in the panel.

**Q: How much does this cost?**  
A: Depends on your API key and model. A batch of 100 assets with Claude Haiku costs a fraction of a cent. You pay only for what you use.

**Q: Can I rename assets without AI?**  
A: Not with this plugin — it's specifically designed for AI-powered suggestions. For manual renaming, use Unreal's native rename tools.

**Q: Does this work with nested folders?**  
A: Yes. Use **Load Folder...** and specify any `/Game/` sub-path. It will scan all assets in that folder and subfolders.

**Q: Where does my API key go?**  
A: Your API key is stored locally in your per-project user settings (`Saved/Config/`), never embedded in the plugin or project files. It is never sent to Epic or anyone else.

---

## Roadmap

- [ ] Batch processing for large projects
- [ ] Custom prefix configuration
- [ ] Dry-run preview (see all changes without applying)
- [ ] Undo last rename batch
- [ ] Integration with naming validation tools

---

## Changelog

### v1.0.0
- Initial release
- 20+ asset type support
- Claude and OpenAI provider support
- One-click rename with preview
- Configurable AI models
- Local API key storage
- Asset sanitization for valid names

---

## Support

- Issues and questions: https://github.com/theworld311/GTW_ProjectDoctor/issues

---

Copyright (c) 2026 GTW Dev. All Rights Reserved.
