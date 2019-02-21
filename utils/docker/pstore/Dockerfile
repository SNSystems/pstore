#===- llvm/tools/pstore/utils/docker/pstore/Dockerfile --------------------===//
#
# Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
# See https://llvm.org/LICENSE.txt for license information.
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
#
#===-----------------------------------------------------------------------===//

# Stage 1. Check out LLVM and PStore source code and run the build.
FROM launcher.gcr.io/google/ubuntu16_04 as builder
#LABEL maintainer "LLVM Developers"

# Install build dependencies of llvm.
# First, Update the apt's source list and include the sources of the packages.
RUN grep deb /etc/apt/sources.list | \
    sed 's/^deb/deb-src /g' >> /etc/apt/sources.list

# Install compiler, python, ninja and cmake.
RUN apt-get update && apt-get install -y --no-install-recommends apt-utils \
    build-essential python cmake git ninja-build && \
    rm -rf /var/lib/apt/lists/*

# Add scripts to create the image.
ADD scripts /tmp/scripts

# Checkout the source code.
ARG checkout_args
RUN echo "checkout_args: " ${checkout_args}
RUN /tmp/scripts/checkout_pstore.sh ${checkout_args}

# Run the build. Results of the build will be available at /tmp/pstore-install/.
ARG buildscript_args
RUN echo "buildscript_args: " ${buildscript_args}
RUN /tmp/scripts/build_install_pstore.sh --to /tmp/pstore-install ${buildscript_args}

# Stage 2. Produce a minimal release image with build results.
FROM launcher.gcr.io/google/ubuntu16_04 as pstore-stage
#LABEL maintainer "LLVM Developers"

# Copy build results of stage 1 to /usr/local.
COPY --from=builder /tmp/pstore-install/ /usr/local/
