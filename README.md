
<p align="center">
  <img src="https://raw.githubusercontent.com/Dronecode/UX-Design/35d8148a8a0559cd4bcf50bfa2c94614983cce91/QGC/Branding/Deliverables/QGC_RGB_Logo_Horizontal_Positive_PREFERRED/QGC_RGB_Logo_Horizontal_Positive_PREFERRED.svg" alt="QGroundControl Logo" width="500">
</p>

<p align="center">
  <a href="https://github.com/mavlink/QGroundControl/releases">
    <img src="https://img.shields.io/github/v/release/mavlink/QGroundControl" alt="Latest Release">
  </a>
</p>

*QGroundControl* (QGC) is a highly intuitive and powerful Ground Control Station (GCS) designed for UAVs. Whether you're a first-time pilot or an experienced professional, QGC provides a seamless user experience for flight control and mission planning, making it the go-to solution for any *MAVLink-enabled drone*.

---

### 🌟 *Why Choose QGroundControl?*

- *🚀 Ease of Use*: A beginner-friendly interface designed for smooth operation without sacrificing advanced features for pros.
- *✈️ Comprehensive Flight Control*: Full flight control and mission management for *PX4* and *ArduPilot* powered UAVs.
- *🛠️ Mission Planning*: Easily plan complex missions with a simple drag-and-drop interface.

🔍 For a deeper dive into using QGC, check out the [User Manual](https://docs.qgroundcontrol.com/en/) – although thanks to QGC's intuitive UI, you may not even need it!

---

### 🚁 *Key Features*

- 🕹️ *Full Flight Control*: Supports all *MAVLink drones*.
- ⚙️ *Vehicle Setup*: Tailored configuration for *PX4* and *ArduPilot* platforms.
- 🔧 *Fully Open Source*: Customize and extend the software to suit your needs.

🎯 Check out the latest updates in our [New Features and Release Notes](https://github.com/mavlink/qgroundcontrol/blob/master/CHANGELOG.md).

---

### 💻 *Get Involved!*

QGroundControl is *open-source*, meaning you have the power to shape it! Whether you're fixing bugs, adding features, or customizing for your specific needs, QGC welcomes contributions from the community.

🛠️ Start building today with our [Developer Guide](https://dev.qgroundcontrol.com/en/) and [build instructions](https://dev.qgroundcontrol.com/en/getting_started/).

---

### 🔗 *Useful Links*

- 🌐 [Official Website](http://qgroundcontrol.com)
- 📘 [User Manual](https://docs.qgroundcontrol.com/en/)
- 🛠️ [Developer Guide](https://dev.qgroundcontrol.com/en/)
- 💬 [Discussion & Support](https://docs.qgroundcontrol.com/en/Support/Support.html)
- 🤝 [Contributing](.github/CONTRIBUTING.md) ([Dev Guide](https://dev.qgroundcontrol.com/en/contribute/))
- 📜 [License Information](https://github.com/mavlink/qgroundcontrol/blob/master/.github/COPYING.md)

---

With QGroundControl, you're in full command of your UAV, ready to take your missions to the next level.

---

## 🐣 agentGCS — AI-Enhanced Ground Control Station

agentGCS is a fork of QGroundControl with integrated AI agent capabilities powered by local LLM inference (Gemma 4 via llama.cpp). It adds an intelligent mission planning assistant that can understand natural language commands, propose flight plans, and execute guided actions — all with human-in-the-loop safety approval.

See the [full implementation plan](docs/plans/2026-04-23-ai-agent-integration.md) for details on architecture and progress.

---

## 🐳 Building with Docker

The Docker container makes it easy to build agentGCS without installing Qt, CMake, or system dependencies locally. The container is located in the `./deploy/docker` directory and is based on **Ubuntu 24.04**. It pre-installs all dependencies at build time, including Qt, using scripts from `./tools/setup`.

### Building the Container

**Using the script** (recommended):

```bash
./deploy/docker/run-docker-ubuntu.sh
```

This builds the Docker image and then immediately runs the container to compile QGC. Build artifacts end up in `./build/`.

**Manually:**

Build the Docker image first:

```bash
docker build --file ./deploy/docker/Dockerfile-build-ubuntu -t qgc-ubuntu-docker .
```

> 💡 The `-t` flag tags the image for later reference. You can have multiple builds of the same container with different tags.
>
> 💡 If building on a Mac with an M-series chip, you must also specify `--platform linux/x86_64`:
> ```bash
> docker build --platform linux/x86_64 --file ./deploy/docker/Dockerfile-build-ubuntu -t qgc-ubuntu-docker .
> ```
> Otherwise you will get an error like: `qemu-x86_64: Could not open '/lib64/ld-linux-x86-64.so.2'`

### Building QGC using the Container

To build QGC, create a `build` directory and run the Docker image using the tag from above, from the root directory:

```bash
mkdir build
docker run --rm -v ${PWD}:/project/source -v ${PWD}/build:/project/build qgc-ubuntu-docker
```

> 💡 For up-to-date Docker command and options, reference the run script in `deploy/docker`, e.g. `run-docker-ubuntu.sh`.
>
> 💡 On Windows, reference `PWD` differently:
> ```bash
> docker run --rm -v %cd%:/project/source -v %cd%/build:/project/build qgc-ubuntu-docker
> ```

Depending on your system resources, the build step can take some time. Subsequent builds are incremental and much faster since the `build` directory persists as a mounted volume.

**Build types** — pass as an argument to the container:

```bash
docker run --rm -v ${PWD}:/project/source -v ${PWD}/build:/project/build qgc-ubuntu-docker Debug
```

- `Release` (default) — Optimized production build
- `Debug` — Full debug symbols, no optimization
- `RelWithDebInfo` — Optimized with debug info
- `MinSizeRel` — Minimum size release

**Using `just` or `make`:**

```bash
just docker   # or: make docker
```

Both invoke `./deploy/docker/run-docker-ubuntu.sh`.

### Running the Built Binary

```bash
# The binary is in ./build/ after a successful build
ls build/QGroundControl

# Run with X11 forwarding (Linux):
xhost +local:docker
docker run --rm -it \
    --env DISPLAY=$DISPLAY \
    --volume /tmp/.X11-unix:/tmp/.X11-unix \
    -v ${PWD}:/project/source \
    -v ${PWD}/build:/project/build \
    qgc-ubuntu-docker

# Or run the binary directly on the host if Qt is installed
```

For GUI display:
- **Linux:** `xhost +local:docker`
- **macOS:** Install [XQuartz](https://www.xquartz.org/) and enable "Allow connections from network clients"
- **Windows:** Use [VcXsrv](https://sourceforge.net/projects/vcxsrv/) or WSL2 with WSLg

### Building with FUSE Support (AppImage)

```bash
./deploy/docker/run-docker-ubuntu.sh --fuse

# Or manually:
docker run --rm \
    --cap-add SYS_ADMIN \
    --device /dev/fuse \
    --security-opt apparmor:unconfined \
    -v ${PWD}:/project/source \
    -v ${PWD}/build:/project/build \
    qgc-ubuntu-docker
```

### What's Inside the Container

The Docker image (`Dockerfile-build-ubuntu`) is based on **Ubuntu 24.04** and includes:

- **Qt 6.10.2** (with modules: qtcharts, qtlocation, qtpositioning, qtspeech, qt5compat, qtmultimedia, qtserialport, qtimageformats, qtshadertools, qtconnectivity, qtquick3d, qtsensors, qtscxml, qtwebsockets, qthttpserver)
- **CMake** + **Ninja** build system
- **Build essentials** (gcc, g++, make)
- **Python 3** (for build scripts and dependency management)
- Git (with safe.directory configured for the mounted volume)

### Troubleshooting

**`bash\r`: No such file or directory** — Linux scripts running with Windows line endings:

```bash
git config --global core.autocrlf false
git rm --cached -r .
git reset --hard
```

Then rebuild the Docker image

---

### Stargazers over time

[![Stargazers over time](https://starchart.cc/mavlink/qgroundcontrol.svg?variant=adaptive)](https://starchart.cc/mavlink/qgroundcontrol)
