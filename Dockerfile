# Build Stage
FROM ubuntu:22.04 AS builder

# Install build dependencies
RUN apt-get update && apt-get install -y \
  build-essential \
  cmake \
  git \
  liburing-dev \
  pkg-config \
  && rm -rf /var/lib/apt/lists/*

WORKDIR /app

# Copy source code
COPY . .

# Build
RUN mkdir build && cd build && \
  cmake -DCMAKE_BUILD_TYPE=Release .. && \
  make -j$(nproc)

# Runtime Stage
FROM ubuntu:22.04

# Install runtime dependencies
RUN apt-get update && apt-get install -y \
  liburing2 \
  && rm -rf /var/lib/apt/lists/*

WORKDIR /app

# Create data directory for persistence
RUN mkdir -p data

# Copy artifacts from builder
COPY --from=builder /app/build/bin/quine-server /usr/local/bin/quine-server

# Expose Redis port
EXPOSE 6379

# Volume for persistence
VOLUME ["/app/data"]

# Entrypoint
ENTRYPOINT ["quine-server"]
CMD ["quine-server"]
