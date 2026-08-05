#!/usr/bin/env bash
set -euo pipefail

LOGFILE="/workspaces/uhtc-nanodb-/setup_cuda_env.log"
exec > >(tee -a "$LOGFILE") 2>&1

info() { echo -e "[INFO] $*"; }
error() { echo -e "[ERROR] $*"; exit 1; }

info "Detecting environment..."
OS="$(. /etc/os-release && echo "$ID")"
ARCH="$(uname -m)"
HAS_NVIDIA_SMI="$(command -v nvidia-smi || true)"
HAS_NVCC="$(command -v nvcc || true)"
info "OS=$OS ARCH=$ARCH HAS_NVIDIA_SMI=$HAS_NVIDIA_SMI HAS_NVCC=$HAS_NVCC"

install_prereqs() {
    info "Updating apt and installing prerequisites..."
    sudo apt-get update -qq
    sudo apt-get install -y --no-install-recommends \
        ca-certificates curl wget git gnupg lsb-release \
        build-essential cmake pkg-config \
        libboost-all-dev libtbb-dev \
        libopenexr-dev libilmbase-dev \
        libpng-dev libjpeg-dev \
        libx11-dev libxi-dev libgl-dev libglew-dev libglfw3-dev \
        libcub-dev software-properties-common
}

install_cuda() {
    info "Setting up CUDA apt repository..."
    CODENAME="$(. /etc/os-release && echo "$VERSION_CODENAME")"
    wget -q https://developer.download.nvidia.com/compute/cuda/repos/ubuntu${CODENAME}/x86_64/cuda-ubuntu${CODENAME}.pin \
      || wget -q https://developer.download.nvidia.com/compute/cuda/repos/ubuntu2204/x86_64/cuda-ubuntu2204.pin
    sudo mv cuda-ubuntu*.pin /etc/apt/preferences.d/cuda-repository-pin-600
    sudo apt-key adv --fetch-keys https://developer.download.nvidia.com/compute/cuda/repos/ubuntu${CODENAME}/x86_64/3bf863cc.pub \
      || sudo apt-key adv --fetch-keys https://developer.download.nvidia.com/compute/cuda/repos/ubuntu2204/x86_64/3bf863cc.pub
    sudo add-apt-repository -y "deb https://developer.download.nvidia.com/compute/cuda/repos/ubuntu${CODENAME}/x86_64/ /" \
      || sudo add-apt-repository -y "deb https://developer.download.nvidia.com/compute/cuda/repos/ubuntu2204/x86_64/ /"
    sudo apt-get update -qq
    info "Installing CUDA toolkit..."
    sudo apt-get install -y --no-install-recommends nvidia-cuda-toolkit
}

verify_cuda() {
    info "Verifying CUDA installation..."
    if ! command -v nvcc >/dev/null 2>&1; then
        error "nvcc not found after installation"
    fi
    nvcc --version | head -1
    if [ -n "$HAS_NVIDIA_SMI" ] || command -v nvidia-smi >/dev/null 2>&1; then
        nvidia-smi -L || true
    else
        info "nvidia-smi not available; runtime GPU tests will be skipped."
    fi
}

build_project() {
    info "Building UHTC-NanoDB..."
    cd /workspaces/uhtc-nanodb-
    mkdir -p build
    cd build
    cmake .. -DCMAKE_BUILD_TYPE=Release
    cmake --build . -j"$(nproc)"
}

run_tests() {
    info "Running tests..."
    cd /workspaces/uhtc-nanodb-/build
    ctest --output-on-failure || true
}

run_analysis() {
    info "Running Python analysis..."
    python3 /workspaces/uhtc-nanodb-/analysis/uhtc_analysis.py || true
}

main() {
    info "=== UHTC-NanoDB CUDA Environment Setup ==="
    install_prereqs
    if [ -z "$HAS_NVCC" ]; then
        install_cuda
    else
        info "CUDA already installed, skipping."
    fi
    verify_cuda
    build_project
    run_tests
    run_analysis
    info "=== Setup complete. Log: $LOGFILE ==="
}

main "$@"
