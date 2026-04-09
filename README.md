# ProbeMaestro.QtInspector

## CI Helper

The GitHub Actions workflow delegates AppVeyor orchestration to [.github/scripts/appveyor_cli.py](.github/scripts/appveyor_cli.py).

The utility uses only the Python standard library and currently covers the AppVeyor operations this repository needs:

- updating project build scripts through the JSON project settings API
- starting a build for the current GitHub branch or pull request
- polling for build completion and printing the tail of failed job logs

The workflow uses the composite command below so AppVeyor-specific API details stay out of the YAML file:

```bash
python .github/scripts/appveyor_cli.py run-cmake-build --compiler msvc --compiler-version 2015 --architecture x64
```

The CLI accepts a normalized compiler description and converts it internally to the AppVeyor-specific CMake generator and platform settings it needs. An explicit `--generator` flag is still available as an escape hatch.

By default the CLI reads `APPVEYOR_ACCOUNT`, `APPVEYOR_PROJECT`, `APPVEYOR_TOKEN`, `GITHUB_HEAD_REF`, and `GITHUB_REF_NAME` from the environment.

