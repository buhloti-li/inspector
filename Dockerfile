# Multi-stage build for Industrial Workcell Server
FROM ubuntu:24.04 AS builder

RUN apt-get update && apt-get install -y --no-install-recommends \
    cmake ninja-build g++ \
    qt6-base-dev qt6-base-dev-tools \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /src
COPY . .

RUN cmake -S . -B build -G Ninja \
    -DCMAKE_BUILD_TYPE=Release \
    -DBUILD_SERVER=ON \
    && cmake --build build --parallel \
    && ctest --test-dir build --output-on-failure

# --- Runtime image ---
FROM ubuntu:24.04

RUN apt-get update && apt-get install -y --no-install-recommends \
    libqt6core6t64 libqt6network6t64 \
    && rm -rf /var/lib/apt/lists/*

COPY --from=builder /src/build/industrial-server /usr/local/bin/industrial-server

EXPOSE 9600

ENTRYPOINT ["industrial-server"]
CMD ["--sim", "--port", "9600"]
