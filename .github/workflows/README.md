# CI

[`ci.yaml`](ci.yaml) runs two jobs on every PR and push to main, on Linux only, since macOS minutes bill ten times over on a private repo.

- `messages` installs the default pixi environment and runs `pixi run setup`, which builds Cyclone DDS and compiles every generated message type, so a change to `Scripts/` or a pixi bump that breaks generation fails here.
- `lint` installs the `lint` environment and runs `pixi run -e lint lint`: ruff on the Python and clang-format on the C++, both checking only. `pixi run -e lint format` fixes what it can. The shader isn't formatted, since clang-format doesn't understand HLSL.

Hosted runners have no Unreal, so the plugin itself is only built and checked locally (see `AGENTS.md`), and there is no clang-tidy, which needs the engine's compile commands.

## Releases

[`release.yaml`](release.yaml) runs when a `v*` tag is pushed. It fails unless the tag is `v` plus `VersionName` from `RosBridge.uplugin`, then creates a GitHub release whose notes git-cliff builds from the conventional commits since the previous tag ([`cliff.toml`](../../cliff.toml)).

To cut one, bump `VersionName` and `Version` in `RosBridge.uplugin`, merge it, then tag that commit:

```sh
git tag v0.1.0 && git push origin v0.1.0
```

The release carries notes only, no built plugin. Zips with Cyclone DDS and the messages prebuilt wait on a license and on confirming which glibc Unreal's Linux toolchain links against.
