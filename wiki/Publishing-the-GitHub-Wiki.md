# Publishing the GitHub Wiki

These files are written as GitHub Wiki Markdown pages. The main repo can keep `wiki/` as the source of truth, while GitHub serves the published copy from the special wiki git repository.

## When the wiki repo exists

From a clean working folder outside this repo:

```powershell
git clone https://github.com/MKme/xcam.wiki.git C:\GitHub\xcam.wiki
Copy-Item C:\GitHub\xcam\wiki\*.md C:\GitHub\xcam.wiki\ -Force
Set-Location C:\GitHub\xcam.wiki
git add .
git commit -m "Create XCAM product and software wiki"
git push origin master
```

If the wiki repository uses `main` instead of `master`, push the branch GitHub created.

## If clone still says repository not found

Open the GitHub web UI for `MKme/xcam`, go to the Wiki tab, and create any first page. GitHub creates the backing `MKme/xcam.wiki.git` repository after that first page exists. Then replace the generated page with these source-controlled pages.

## Source of truth

Keep these files in `wiki/` as the source of truth, then republish them into the GitHub Wiki after edits. That keeps wiki changes reviewable in normal firmware commits.

## Pages to copy

```text
Home.md
Product-Ecosystem.md
System-Architecture.md
XCAM-Firmware.md
Hardware-Targets.md
LAN-Video-and-XTOC-ISR.md
Motion-Alerts-and-Sentinel-Bridge.md
Build-Flash-Test-and-CI.md
Operations-and-Troubleshooting.md
Publishing-the-GitHub-Wiki.md
_Sidebar.md
```
