# Workstation setup (UE 5.4.4)

Order: prepare the PC → Visual Studio 2022 → Unreal Engine 5.4.4 → Git → clone this repository → build → open.
Steps 2 and 3 are the long downloads; run them at the same time.

## 1. Prepare the PC

- About 120 GB free on an SSD.
- Windows Update and the latest graphics driver installed.
- Create `C:\Dev`. Keep the project there, **never in OneDrive or a synced Documents folder**.
- Allow long paths: PowerShell **as Administrator**:
  ```powershell
  New-ItemProperty -Path "HKLM:\SYSTEM\CurrentControlSet\Control\FileSystem" -Name "LongPathsEnabled" -Value 1 -PropertyType DWORD -Force
  ```
- Optional, faster builds: Windows Security → Virus & threat protection → Exclusions → add folder `C:\Dev`.

## 2. Visual Studio 2022 (Community, free)

UE 5.4 is built for Visual Studio 2022 and the **MSVC 14.38** compiler. Newer MSVC versions are known to break UE 5.4
builds, so install 14.38 alongside whatever Visual Studio installs by default.

1. Install **Visual Studio 2022 Community** (17.8 or later).
2. Workloads: **Game development with C++** and **Desktop development with C++**.
   Under Game development with C++ also tick **Unreal Engine installer** and a **Windows 10/11 SDK**.
3. **Individual components** tab → search `14.38` → tick
   **MSVC v143 - VS 2022 C++ x64/x86 build tools (v14.38-17.8)**. Also tick **.NET Framework 4.8 targeting pack**.
4. Install. UE 5.4 picks 14.38 automatically when it is present.

## 3. Unreal Engine 5.4.4

1. Epic Games Launcher → Unreal Engine → Library → **+** → choose **5.4.4** → Install.
2. Options: Core Components on, Starter Content off, Templates and Feature Packs on, Engine Source off,
   Editor symbols off, Target platforms: Windows only.
3. Launch it once and close it (first start compiles shaders).

## 4. Git and Git LFS

1. Install **Git for Windows** (git-scm.com). Defaults are fine; it includes Git LFS and the credential manager.
2. In PowerShell:
   ```powershell
   git lfs install
   git config --global user.name "Your Name"
   git config --global user.email "you@example.com"
   git config --global core.longpaths true
   ```
3. Optional: **GitHub Desktop** for committing with a few clicks (File → Add local repository → `C:\Dev\Aftermath`).

GitHub's free plan includes 10 GiB of LFS storage and 10 GiB of LFS download per month. That covers the project for
a long time; when scanned materials and firearm models push past it, buy LFS capacity in GitHub billing.

## 5. Clone, build, open

```powershell
cd C:\Dev
git clone https://github.com/andr0n0ss7/aftermath.git Aftermath
cd Aftermath
git lfs pull
```

1. Right-click `Aftermath.uproject` → Show more options → **Generate Visual Studio project files**.
2. Open `Aftermath.sln`, set **Development Editor** and **Win64**, Build → Build Solution.
   On errors: View → Error List, copy everything, send it over.
3. Double-click `Aftermath.uproject`. Python and Editor Scripting plugins are already enabled by the project.
4. Tools → Execute Python Script → `Plugins/AftermathCore/Content/Python/aftermath_setup.py`.
5. Build → Build Lighting Only → Play.
6. Commit the generated assets:
   ```powershell
   git add -A
   git commit -m "Generated Kitchen level and assets"
   git push
   ```

## Daily routine

1. `git pull` before starting.
2. Work. Close the editor before committing (it locks files).
3. `git add -A`, `git commit -m "one line on what changed"`, `git push`.
4. Undo everything since the last commit: `git restore .` then `git clean -fd` (deletes new files too).

## Checks

- [ ] Compiles; Session Frontend → Automation → "Aftermath": all tests pass.
- [ ] All 19 traces exist while playing (`Evidence_E01` … `Evidence_E19`).
- [ ] 60 fps at 1080p on Medium on the 4 GB test card (`stat fps` in the console).
- [ ] `-dx12` launch also works.
